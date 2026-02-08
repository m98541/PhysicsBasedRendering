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
void JointsDataSort(Joint_T* data, size_t size);


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
		this->nodeToJointIdTable[node.id.c_str()] = i;
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

void MSGLTFLoader::GetMeshResource(MeshResource_T &outData)
{

	const Skin& srcSkin = document.skins.Get(0);

	size_t nodeMaxCount = document.nodes.Size();
	vector<const Node*> skinNodes;
	skinNodes.reserve(nodeMaxCount);

	for (int i = 0; i < nodeMaxCount; ++i)
	{
		const Node& node = document.nodes.Get(i);

		
		if (node.skinId == srcSkin.id)
		{
			skinNodes.push_back(&node);
		}
	}

	outData.count = skinNodes.size();
	outData.array = new Mesh_T[outData.count];


	vector<float> posData;
	vector<float> normData;
	vector<float> texData;
	vector<uint32_t> indicesData;
	vector<uint16_t> inputIndicesData;
	vector<uint32_t> globalIndicesData;

	for (int i = 0; i < outData.count; ++i)
	{
		const Node* node = skinNodes[i];
		
		const Mesh& mesh = document.meshes.Get(node->meshId);

		outData.array[i].jointId = nodeToJointIdTable[node->id.c_str()];
		
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

			auto posIt = mesh.primitives[j].attributes.find("POSITION");
			auto normIt = mesh.primitives[j].attributes.find("NORMAL");
			auto texIt = mesh.primitives[j].attributes.find("TEXCOORD_0");

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
		}
		outData.array[i].indicesCount = globalIndicesData.size();
		outData.array[i].indices = new uint32_t[outData.array[i].indicesCount];
		memcpy(outData.array[i].indices , globalIndicesData.data() , sizeof(uint32_t) * outData.array[i].indicesCount);
		globalIndicesData.clear();
	}
}

void MSGLTFLoader::GetCharacterResource(CharacterResource_T* resourceData)
{
	
	GetSkeletonResource(resourceData->skeletonResource);
	
	JointsDataSort(resourceData->skeletonResource.array ,resourceData->skeletonResource.count);
	
	for (int i = 0; i < resourceData->skeletonResource.count; ++i)
	{// 정렬되어 변경된 array 의 index 에 맞게 nodeToJointIdTable 업데이트
		nodeToJointIdTable[ resourceData->skeletonResource.array[i].nodeId.c_str() ] = i;
	}

	GetMeshResource(resourceData->meshResource);

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

void JointsDataSort(Joint_T* data ,size_t size)
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

}