#include "MS_GLTFLoader.h"
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <assert.h>

using namespace Microsoft::glTF;
using namespace std;

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
	vector<float> inverseMatData = resourceReader->ReadBinaryData<float>(document,skinAccessor);

	outData.count = srcSkin.jointIds.size();
	outData.array = (Joint_T*)malloc(sizeof(Joint_T) * outData.count);
	
	int matrixSize = sizeof(DirectX::XMFLOAT4X4);
	
	for (int i = 0; i < outData.count; i++)
	{
		memcpy(&outData.array[i].inverseBindPose, 
			&inverseMatData[i * matrixSize] 
			, matrixSize);

		const Node& node = document.nodes.Get(srcSkin.jointIds[i]);
		outData.array[i].jointName = node.name;
		
		outData.array[i].childrenCount = document.nodes.Get(srcSkin.jointIds[i]).children.size();
		outData.array[i].childrenIndices = (unsigned int*)malloc( outData.array[i].childrenCount * sizeof(unsigned int));

		memcpy(&outData.array[i].childrenIndices , 
			&document.nodes.Get(srcSkin.jointIds[i]).children ,
			outData.array[i].childrenCount);

		outData.array[i].parentIndex;
		

		
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