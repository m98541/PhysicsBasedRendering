#include "MS_GLTFLoader.h"
#include <assert.h>
constexpr float WEIGHT_EPSILON = 1e-5f;
using namespace DirectX;
using namespace Microsoft::glTF;
using namespace std;

typedef struct SortBucket_S
{
	int originIndex;
	int updateParent;
}SortBucket_T;

string ReadFile(const string& path);
class StreamReader : public IStreamReader
{
	public:
		shared_ptr<istream> GetInputStream(const string& path) const override
		{
			return make_shared<ifstream>(path , ios::binary);
		}
};

MSGLTFLoader::MSGLTFLoader()
{

}

MSGLTFLoader::MSGLTFLoader(const string& path)
{	
	
	this->LoadModel(path);
}

MSGLTFLoader::~MSGLTFLoader()
{

}



bool MSGLTFLoader::LoadModel(const string& path)
{
	auto streamReader = make_shared<StreamReader>();
	this->resourceReader = make_unique<GLTFResourceReader>(streamReader);

	string file = ReadFile(path);
	this->document = Deserialize(file);

	
	return true;
}


void MSGLTFLoader::GetSkeletonResource(SkeletonResource_T& outData)
{   
	// GLTF 데이터 -> 스켈레톤 자원으로 변환 
	const Skin& srcSkin = document.skins.Get(0);
	const Accessor& skinAccessor = document.accessors.Get(srcSkin.inverseBindMatricesAccessorId);
	vector<float> inverseMatData = resourceReader->ReadFloatData(document,skinAccessor);

	outData.count = srcSkin.jointIds.size();
	outData.array = new Joint_T[outData.count];
	
	int matrixSize = sizeof(DirectX::XMFLOAT4X4);
	//string 형태 ID를 키로 , i 번째 의 index를 value로
	eastl::map<eastl::string, int> jointMap;
	
	for (int i = 0; i < outData.count; ++i)
	{
		memcpy(&outData.array[i].inverseBindPose, 
			&inverseMatData[i * 16] 
			, matrixSize);

		const Node& node = document.nodes.Get(srcSkin.jointIds[i]);
		jointMap[srcSkin.jointIds[i].c_str()] = i;
		outData.array[i].jointId = srcSkin.jointIds[i].c_str();
		outData.array[i].nodeId = node.id;
		outData.array[i].jointName = node.name;
		outData.array[i].parentIndex = 0;

		outData.array[i].jointLocalPose.quatRot = { node.rotation.x ,node.rotation.y , node.rotation.z , node.rotation.w};
		outData.array[i].jointLocalPose.trans = {node.translation.x ,node.translation.y , node.translation.z};
		outData.array[i].jointLocalPose.scale = node.scale.x;
	}

	for (int i = 0; i < outData.count; i++)
	{
		const Node& node = document.nodes.Get(srcSkin.jointIds[i]);
		
		int size = node.children.size();

		for (int j = 0; j < size; j++)
		{
			eastl::string jointName(node.children[j].c_str());

			Joint_T* child = &outData.array[jointMap[node.children[j].c_str()]];
			child->parentIndex = i;
		}
	}

	
}

void MSGLTFLoader::GetMeshResource(MeshResource_T &outData , eastl::map<eastl::string, int>& textureIdMap)
{

	const Skin& srcSkin = document.skins.Get(0);

	size_t nodeMaxCount = document.nodes.Size();
	vector<const Node*> skinNodes;
	skinNodes.reserve(nodeMaxCount);

	int test = 0;
	outData.count = 0;
	for (int i = 0; i < nodeMaxCount; ++i)
	{
		const Node& node = document.nodes.Get(i);
		
		if (node.skinId == srcSkin.id)
		{
			outData.count++;
			skinNodes.push_back(&node);
		}
	}


	outData.array = new Mesh_T[outData.count];

	int textureId = 0; // 이후 materialId 로 교체 예정 material 정보 내에 textureID 존재
	vector<float> posData;
	vector<float> normData;
	vector<float> texData;
	vector<uint32_t> indicesData;
	vector<uint16_t> inputIndicesData;
	vector<uint32_t> globalIndicesData;
	vector<uint16_t> jointsData;
	vector<float> weightsData;


	for (int i = 0; i < outData.count; ++i)
	{
		const Node* node = skinNodes[i];
		
		const Mesh& mesh = document.meshes.Get(node->meshId);
		uint32_t meshVertexCount = 0;
		for (int j = 0; j < mesh.primitives.size(); ++j)
		{
			const Accessor& posAccessor = document.accessors.Get(mesh.primitives[j].attributes.find("POSITION")->second);
			meshVertexCount += posAccessor.count;
		}

		uint32_t outDataIndex = 0;
		uint32_t outDataVertexOffset = 0;

		outData.array[i].vertices = new MeshVertex_T[meshVertexCount];
		outData.array[i].verticesCount = meshVertexCount;

		for (int j = 0; j < mesh.primitives.size(); ++j)
		{
			//이후 로직에서 texture를 Material로 확장 확장시에 너무 코드가 무거워짐에 따라 내부를 함수로 쪼개거나 필요...
			if (mesh.primitives[j].materialId.empty())
			{
				textureId = -1;
			}
			else
			{
				const Material& material = document.materials.Get(mesh.primitives[j].materialId);

				if (material.metallicRoughness.baseColorTexture.textureId.empty())
				{
					textureId = -1;

				}
				else
				{
					const Texture& texture = document.textures.Get(material.metallicRoughness.baseColorTexture.textureId);
					auto it = textureIdMap.find(texture.imageId.c_str());

					if (it != textureIdMap.end())
					{
						textureId = it->second;
					}
					else
					{
						assert(false && "Critical Error: Referenced Image ID not found in TextureMap!");
					}
					
				}
				
			}
			
			

			auto posIt = mesh.primitives[j].attributes.find("POSITION");
			auto normIt = mesh.primitives[j].attributes.find("NORMAL");
			auto texIt = mesh.primitives[j].attributes.find("TEXCOORD_0");
			auto jointIt = mesh.primitives[j].attributes.find("JOINTS_0");
			auto weightIt = mesh.primitives[j].attributes.find("WEIGHTS_0");

			mesh.primitives[j].materialId;
			string indicesId = mesh.primitives[j].indicesAccessorId;

			

			int primitiveVertexCount = 0;
			if (posIt != mesh.primitives[j].attributes.end())
			{
				const Accessor& posAccessor = document.accessors.Get(posIt->second);
				posData = resourceReader->ReadFloatData(document, posAccessor);
				primitiveVertexCount = posAccessor.count;
			}
			

			if (normIt != mesh.primitives[j].attributes.end())
			{
				const Accessor& normAccessor = document.accessors.Get(normIt->second);
				normData = resourceReader->ReadFloatData(document, normAccessor);
			}
		

			if (texIt != mesh.primitives[j].attributes.end())
			{
				const Accessor& texAccessor = document.accessors.Get(texIt->second);
				texData = resourceReader->ReadFloatData(document, texAccessor);
			}
			
			if (!indicesId.empty())
			{
				const Accessor& indicesAccessor = document.accessors.Get(indicesId);

				if (indicesAccessor.componentType == ComponentType::COMPONENT_UNSIGNED_SHORT)
				{
					inputIndicesData = resourceReader->ReadBinaryData<uint16_t>(document, indicesAccessor);
					indicesData.assign(inputIndicesData.begin() , inputIndicesData.end());
					inputIndicesData.clear();
				}
				else
				{
					indicesData = resourceReader->ReadBinaryData<uint32_t>(document, indicesAccessor);
				}

			}

			if (jointIt != mesh.primitives[j].attributes.end())
			{
				const Accessor& jointAccessor = document.accessors.Get(jointIt->second);
				jointsData = resourceReader->ReadBinaryData<uint16_t>(document , jointAccessor);
			}
			
			if (weightIt != mesh.primitives[j].attributes.end())
			{
				const Accessor& weightAccessor = document.accessors.Get(weightIt->second);
				weightsData = resourceReader->ReadFloatData(document , weightAccessor);
			}


			for (int k = 0; k < primitiveVertexCount; ++k)
			{

				if (!posData.empty())
				{
					memcpy(&outData.array[i].vertices[outDataIndex].position, &posData[k * 3], sizeof(float) * 3);
					outData.array[i].vertices[outDataIndex].position.w = 1.F;
				}

				if (!normData.empty())
				{
					memcpy(&outData.array[i].vertices[outDataIndex].normal, &normData[k * 3], sizeof(float) * 3);
					outData.array[i].vertices[outDataIndex].normal.w = 0.F;
				}
				else
				{
					memset(&outData.array[i].vertices[outDataIndex].normal, 0, sizeof(float) * 3);
					outData.array[i].vertices[outDataIndex].normal.w = 0.F;
				}
				
				if (!texData.empty())
				{
					memcpy(&outData.array[i].vertices[outDataIndex].uv , &texData[k * 2], sizeof(float) * 2);
					
				}
				else
				{
					memset(& outData.array[i].vertices[outDataIndex].uv, 0, sizeof(float) * 2);
				}
				


				float weightSum = 0;
				
				for (uint32_t boneIdx = 0; boneIdx < 4; ++boneIdx)
				{
					uint16_t srcSkinJointIdIndex = jointsData[k * 4 + boneIdx];
					string srcSkinJointId = srcSkin.jointIds[srcSkinJointIdIndex];

					uint32_t jointsArrIndex = JointIdToIndexTable[srcSkinJointId.c_str()];

					float weight = weightsData[k * 4 + boneIdx];

					outData.array[i].vertices[outDataIndex].joints[boneIdx] = jointsArrIndex;
					
					outData.array[i].vertices[outDataIndex].weights[boneIdx] = weight;

					weightSum += weight;
				}
	
				//weight 정규화
				if (weightSum < WEIGHT_EPSILON)
				{
					assert(weightSum > WEIGHT_EPSILON && "find zero weight vertex");
				}
				else if (fabs(weightSum - 1.0) > WEIGHT_EPSILON)
				{
					for (uint32_t boneIdx = 0; boneIdx < 4; ++boneIdx)
					{
						outData.array[i].vertices[outDataIndex].weights[boneIdx] /= weightSum;
					}
				}
				outData.array[i].vertices[outDataIndex].textureId = textureId;

				outDataIndex++;
			} 







			if (!indicesData.empty())
			{
				int primitiveIndicesCount = indicesData.size();

				for (int k = 0; k < primitiveIndicesCount; ++k)
				{
					globalIndicesData.push_back(indicesData[k] + outDataVertexOffset);
				}
			}
			





			outDataVertexOffset += primitiveVertexCount;
			posData.clear();
			normData.clear();
			texData.clear();
			indicesData.clear();
			jointsData.clear();
			weightsData.clear();
		}
		outData.array[i].indicesCount = globalIndicesData.size();
		outData.array[i].indices = new uint32_t[outData.array[i].indicesCount];
		memcpy(outData.array[i].indices , globalIndicesData.data() , sizeof(uint32_t) * outData.array[i].indicesCount);
		globalIndicesData.clear();
	}


}

void MSGLTFLoader::GetTextureResource(TextureResourec_T& outTexData, eastl::map<eastl::string, int>& textureIdMap)
{
	outTexData.size = document.images.Size();
	outTexData.Images = new DirectX::ScratchImage[outTexData.size];

	vector<uint8_t> tempBinImageBuffer;
	int index = 0;
	for (const auto& srcImage : document.images.Elements())
	{
		tempBinImageBuffer = resourceReader->ReadBinaryData(document , srcImage);

		//binary rgba 변환 - 1

		DirectX::ScratchImage image;
		HRESULT hr = DirectX::LoadFromWICMemory(
			tempBinImageBuffer.data(),
			tempBinImageBuffer.size(),
			DirectX::WIC_FLAGS_NONE,
			nullptr,
			image
		);

		//convert
		printf("\n error code 0x%x \n" , hr);
		if (FAILED(hr))
			assert(FALSE && "Image Load Fail");

		if (image.GetMetadata().format != DXGI_FORMAT_R8G8B8A8_UNORM)
		{
			//convert
			ScratchImage convertImage;
			Convert(
				image.GetImages(),
				image.GetImageCount(),
				image.GetMetadata(),
				DXGI_FORMAT_R8G8B8A8_UNORM,
				TEX_FILTER_DEFAULT,
				TEX_THRESHOLD_DEFAULT,
				convertImage
			);

			outTexData.Images[index] = std::move(convertImage);
		
		}
		else
		{
			// 1 에 의해 새롭게 적용될 변경되어야 할 부분  
			outTexData.Images[index] = std::move(image);
		}


		
		textureIdMap[srcImage.id.c_str()] = index;
		tempBinImageBuffer.clear();
		index++;
	}


}

void MSGLTFLoader::GetCharacterResource(CharacterResource* resourceData)
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	eastl::map<eastl::string, int> textureIdMap;
	
	GetSkeletonResource(resourceData->skeletonResource);
	
	JointsDataSort(resourceData->skeletonResource.array ,resourceData->skeletonResource.count);
	
	
	//텍스처 정보 받음 여기서 이미지 정수 식별자와 문자열 식별자 번역 맵 구성
	GetTextureResource(resourceData->TextureResource, textureIdMap);

	//메쉬 정보 받음
	GetMeshResource(resourceData->meshResource, textureIdMap);

	//에니메이션 정보 받음
	GetAnimationResource(resourceData->animationResource ,resourceData->skeletonResource.count);

	//pos 좌표 정규화 스키닝 때문에 정규화 행렬를 상수 버퍼로 올려서 model * normalizeMat * skinnedMAt * pos 로 연산시킬 예정
	PosDataNormalization(resourceData);

	CoUninitialize();

	
}

void MSGLTFLoader::GetAnimationResource(AnimationResource_T& outData, size_t jointsCount)
{

	//여러개의 에니메이션 클립이 들어옴을 고려해야함
	const uint32_t animationSize = document.animations.Size();
	outData.count = animationSize;
	outData.animationClip = new AnimationClipResourec_T[animationSize];

	size_t animationChannelsSize = 0;

	

	for (uint32_t i = 0; i < animationSize; ++i)
	{
		const Animation& animation = document.animations.Get(i);
		animationChannelsSize = animation.channels.Size();
		outData.animationClip[i].AnimationJoint = new AnimationJoint_T[jointsCount];
		outData.animationClip[i].count = animationChannelsSize;
		outData.animationClip[i].totalTime = 0.F;

		for (size_t j = 0; j < animationChannelsSize; j++)
		{
			AnimationChannel animationChannel = animation.channels[j];
			const AnimationSampler& animationSampler = animation.samplers.Get(animationChannel.samplerId);
			const AnimationTarget& animationTarget = animationChannel.target;

			//data access
			const Accessor& timeAccessor = document.accessors.Get(animationSampler.inputAccessorId);
			const Accessor& valueAccessor = document.accessors.Get(animationSampler.outputAccessorId);

			auto timeData = resourceReader->ReadFloatData(document ,timeAccessor );
			auto valueData = resourceReader->ReadFloatData(document ,valueAccessor);
			

			//target data index search //아직 연결 안된 테이블임 나중에 이글 보면 연결하고 지워 
			uint32_t skeletonArrIndex = nodeIdToIndexTable[animationTarget.nodeId.c_str()];

			//data 입력
			outData.animationClip[i].AnimationJoint[skeletonArrIndex].localPoseArrIndex = skeletonArrIndex;

			uint32_t timeKeyCount = timeData.size();
			if (timeData[timeKeyCount - 1] > outData.animationClip[i].totalTime)
			{
				outData.animationClip[i].totalTime = timeData[timeKeyCount - 1];
			}
 

			if (animationTarget.path == TargetPath::TARGET_TRANSLATION)
			{
				for (uint32_t t = 0; t < timeKeyCount; ++t)
				{
					AnimationKeyTrans_T data = {
						timeData[t],
						{valueData[t * 3 + 0] ,valueData[t * 3 + 1] , valueData[t * 3 + 2] } // translate v3f
					};
					outData.animationClip[i].AnimationJoint[skeletonArrIndex].transKeys.push_back(data);
					
				}
			}
			else if (animationTarget.path == TargetPath::TARGET_ROTATION)
			{
				for (uint32_t t = 0; t < timeKeyCount; ++t)
				{
					AnimationKeyQuatRot_T data = {
						timeData[t],
						{valueData[t * 4 + 0] ,valueData[t * 4 + 1] , valueData[t * 4 + 2] , valueData[t * 4 + 3] } // Quat v4f
					};
					outData.animationClip[i].AnimationJoint[skeletonArrIndex].quatRotKeys.push_back(data);
				}
			}
			else if (animationTarget.path == TargetPath::TARGET_SCALE)
			{
				for (uint32_t t = 0; t < timeKeyCount; ++t)
				{
					AnimationKeyScale_T data = {
						timeData[t],
						{valueData[t * 3 + 0] ,valueData[t * 3 + 1] , valueData[t * 3 + 2] } // 0번째 값만 사용 균일 스케일 1f
					};
					outData.animationClip[i].AnimationJoint[skeletonArrIndex].scaleKeys.push_back(data);
				}
			}

			
		}
	}



}

string ReadFile(const string& path)
{
	ifstream file;  
	string buffer;
	int size;

	file.open(path);
	assert(file && "file read fail!");

	file.seekg(0, ios::end);
	size = file.tellg();
	buffer.resize(size);
	file.seekg(0, ios::beg);

	file.read(&buffer[0], size);
	file.close();

	return buffer;
}

void MSGLTFLoader::JointsDataSort(Joint_T* data ,size_t size)
{
	eastl::queue<SortBucket_T> orderQueue;
	Joint_T* resultBuffer = new Joint_T[size];
	int searchParent = 0;
	int updateParent = 0;
	//loop
	int OrgBufferRootMin = 0;
	int bufferSizeCount = 0;
	
	while (bufferSizeCount != size)
	{
		//root 찾기 Origin 데이터에서 현재 index 와 부모 index 동일시 root 로 간주함
		int root = OrgBufferRootMin;
		for (root;
			root < size &&
			root != data[root].parentIndex &&
			root <= OrgBufferRootMin;
			root++);

		if (root > OrgBufferRootMin) OrgBufferRootMin = root;

		SortBucket_T rootJoint = { root  ,bufferSizeCount };
		orderQueue.push(rootJoint);

		//root 하위영역 찾기
		while (!orderQueue.empty())// 큐가 비었다 leap 까지 모두 도달 but  bufferSizeCount != size 면 root 여러개
		{
			SortBucket_T reciveBucket = orderQueue.front();
			orderQueue.pop();

			
			resultBuffer[bufferSizeCount] = data[reciveBucket.originIndex];

			//참조! 추가된 부분 
			JointIdToIndexTable[data[reciveBucket.originIndex].jointId.c_str()] = bufferSizeCount;
			nodeIdToIndexTable[data[reciveBucket.originIndex].nodeId.c_str()] = bufferSizeCount;
			resultBuffer[bufferSizeCount].parentIndex = reciveBucket.updateParent;
			


			XMMATRIX transposeMat4X4 = XMLoadFloat4x4(&resultBuffer[bufferSizeCount].inverseBindPose);
			//transposeMat4X4 = XMMatrixTranspose(transposeMat4X4);
			XMStoreFloat4x4(&resultBuffer[bufferSizeCount].inverseBindPose, transposeMat4X4);

			updateParent = bufferSizeCount++;

			searchParent = reciveBucket.originIndex;
			for (int i = 0; i < size; ++i)
			{
				if (i != data[i].parentIndex &&
					searchParent == data[i].parentIndex)
				{
					SortBucket_T sendBucket = { i  , updateParent };
					orderQueue.push(sendBucket);
				}
			}
		}


	}
	//정렬된 데이터 셋 기존 데이터와 교체
	memcpy(data, resultBuffer, size * sizeof(Joint_T));
	delete[] resultBuffer;
}

void MSGLTFLoader::PosDataNormalization(CharacterResource* resourceData)
{	
	if (resourceData->meshResource.count == 0) return;

	XMFLOAT4 max = resourceData->meshResource.array[0].vertices[0].position;
	XMFLOAT4 min = max;

	for (uint32_t i = 0; i < resourceData->meshResource.count; ++i)
	{
		for (uint32_t j = 0; j < resourceData->meshResource.array[i].verticesCount; ++j)
		{
			const XMFLOAT4& pos = resourceData->meshResource.array[i].vertices[j].position;

			if (pos.x > max.x) max.x = pos.x;
			else if (pos.x < min.x) min.x = pos.x;

			if (pos.y > max.y) max.y = pos.y;
			else if (pos.y < min.y) min.y = pos.y;

			if (pos.z > max.z) max.z = pos.z;
			else if (pos.z < min.z) min.z = pos.z;

		}
	}
	XMVECTOR diffV = XMLoadFloat4(&max) - XMLoadFloat4(&min);
	diffV = XMVectorAbs(diffV);
	XMFLOAT4 diff;
	XMStoreFloat4(&diff, diffV);

	float maxDiff = diff.x;
	if (diff.y > maxDiff) maxDiff = diff.y;
	if (diff.z > maxDiff) maxDiff = diff.z;

	XMFLOAT3 center = {
		(max.x + min.x) * 0.5F,
		(max.y + min.y) * 0.5F,
		(max.z + min.z) * 0.5F
	
	};
	float scale = 2.F / maxDiff;
	//모델 정규화 행렬 [-1 , 1] NDC 로 
	resourceData->modelNDCMat = {
		scale , 0 , 0 , 0 ,
		0, scale , 0, 0,
		0 , 0 , scale , 0,
		(-center.x * scale) , (-center.y * scale) , (-center.z * scale), 1.F
	};




}