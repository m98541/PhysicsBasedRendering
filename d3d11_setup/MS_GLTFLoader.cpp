#include "MS_GLTFLoader.h"
#include <assert.h>

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

		if (node.skinId == "")
		{
			test++;
		}
		
		if (node.skinId == srcSkin.id)
		{
			outData.count++;
			skinNodes.push_back(&node);
		}
	}


	outData.array = new Mesh_T[outData.count];

	int textureId; // 이후 materialId 로 교체 예정 material 정보 내에 textureID 존재
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

				if (material.emissiveTexture.textureId.empty())
				{
					textureId = -1;
				}
				else
				{
					const Texture& texture = document.textures.Get(material.emissiveTexture.textureId);
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
					memcpy(&outData.array[i].vertices[outDataIndex].uv, &texData[k * 2], sizeof(float) * 2);
				}
				else
				{
					memset(& outData.array[i].vertices[outDataIndex].uv, 0, sizeof(float) * 2);
				}
				


				for (uint32_t boneIdx = 0; boneIdx < 4; ++boneIdx)
				{
					uint16_t srcSkinJointIdIndex = jointsData[k * 4 + boneIdx];
					string srcSkinJointId = srcSkin.jointIds[srcSkinJointIdIndex];

					uint32_t jointsArrIndex = JointIdToIndexTable[srcSkinJointId.c_str()];

					float weight = weightsData[k * 4 + boneIdx];

					outData.array[i].vertices[outDataIndex].joints[boneIdx] = jointsArrIndex;
					outData.array[i].vertices[outDataIndex].weights[boneIdx] = weight;
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

		printf("\n error code 0x%x \n" , hr);
		if (FAILED(hr))
			assert(FALSE && "Image Load Fail");

		// 1 에 의해 새롭게 적용될 변경되어야 할 부분  
		outTexData.Images[index] = std::move(image);
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

	CoUninitialize();

	
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

			resultBuffer[bufferSizeCount].parentIndex = reciveBucket.updateParent;
			


			XMMATRIX transposeMat4X4 = XMLoadFloat4x4(&resultBuffer[bufferSizeCount].inverseBindPose);
			transposeMat4X4 = XMMatrixTranspose(transposeMat4X4);
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