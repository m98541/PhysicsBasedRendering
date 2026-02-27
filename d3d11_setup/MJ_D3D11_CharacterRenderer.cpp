#include "MJ_D3D11_CharacterRenderer.h"
#include <DirectXMath.h>
#include <EASTL/vector.h>

// 해당 character shdaer 내부 input layout 에 매칭 되는 정점 구조
constexpr uint32_t g_CharacterInputElementCount = 6;
D3D11_INPUT_ELEMENT_DESC g_CharacterInputElement[g_CharacterInputElementCount] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORM",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXIDX",   0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"JOINTS",   0, DXGI_FORMAT_R32G32B32A32_UINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"WEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}
};


/*

typedef struct MeshVertex_S
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 uv;
	uint32_t textureId;// 이후 material ID 로 교체 예정 material 정보 내에 textureID 존재
	uint32_t joints[4];
	float weights[4];
}MeshVertex_T;
*/
// input 정보는 mesh정보의 확장형 




struct Cam_S
{
	DirectX::XMMATRIX view;
	DirectX::XMMATRIX proj;
};

struct Joints_S
{
	DirectX::XMMATRIX joints[128];
	uint32_t activeJointsCount;
};

void CreateSRVArrayFromTextureResource(ID3D11Device* dev, const TextureResourec_T* textureRrsData, ID3D11ShaderResourceView** textureSRV);

CharacterRenderer::CharacterRenderer()
{
	jointsMatrixBuffer = nullptr;
	characterVertexShader = nullptr;
	characterPixelShader = nullptr;
	pCamBuffer = nullptr;
	pModelBuffer = nullptr;
	pJointsBuffer = nullptr;
	
}

CharacterRenderer::CharacterRenderer(ID3D11Device* dev, ID3D11DeviceContext* devCon, Character& character)
{

	ShaderFile* vsShader = new ShaderFile(L"Character_Vertex_Shader.hlsl","main","vs_5_0",VERTEX_SHADER_FILE);
	ShaderFile* psShader = new ShaderFile(L"Character_Pixel_Shader.hlsl","main","ps_5_0",PIXEL_SHADER_FILE);
	
	
	InitPipeLine(dev , devCon , *vsShader ,*psShader , g_CharacterInputElement , g_CharacterInputElementCount);

	InitData(dev , devCon , character);

	CBInit(dev);

	DirectX::XMMATRIX initModel = character.GetModelMatrix();
	const DirectX::XMFLOAT4X4* joints = character.GetCurrentJointsMatrix(&jointsCount);
	const DirectX::XMFLOAT4X4* inverseJoints = character.GetInverseJoints(&jointsCount);
	jointsMatrixBuffer = new DirectX::XMMATRIX[jointsCount];

	for (int i = 0; i < jointsCount; ++i)
	{
		jointsMatrixBuffer[i] =  DirectX::XMLoadFloat4x4(&joints[i]) *  DirectX::XMLoadFloat4x4(&inverseJoints[i]);
	}

	CBJointsUpdate(devCon, jointsMatrixBuffer, jointsCount);

	CBModelUpdate(devCon , initModel);
	
}


CharacterRenderer::~CharacterRenderer()
{
	characterVertexShader->Release();
	characterPixelShader->Release();
}

void CharacterRenderer::InitPipeLine(
	ID3D11Device* dev, 
	ID3D11DeviceContext* devCon, 
	ShaderFile& vsFile, 
	ShaderFile& psFile, 
	D3D11_INPUT_ELEMENT_DESC* inputElement, 
	int inputElementCount)
{
	characterVertexShader = vsFile.VertexShaderCompile(dev);
	characterPixelShader = psFile.PixelShaderCompile(dev);
	if ( vsFile.GetBlobSize() == 0)
	{
		printf("HLSL 컴파일 실패! 셰이더 코드를 확인하세요.\n");
		assert(false && "Vertex Shader Blob is NULL!");
	}


	
	HRESULT hr =  dev->CreateInputLayout(inputElement, inputElementCount, vsFile.GetBufferPointer(), vsFile.GetBlobSize(), &inputLayout);

	if (FAILED(hr))
	{
		// 여기서 브레이크포인트가 걸린다면 C++ InputLayout과 HLSL Signature가 불일치하는 것입니다.
		printf("Character Input Layout Create Failed!\n");
		assert(false && "Input Layout Mismatch!");
	}

	this->SetPipeLine(devCon);

}


void CharacterRenderer::InitData(ID3D11Device* dev, ID3D11DeviceContext* devCon, Character& character)
{
	
	//vertice indices data init
	D3D11_BUFFER_DESC vertexBufferDesc = {};
	memset(&vertexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	D3D11_BUFFER_DESC indexBufferDesc = {};
	memset(&indexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	

	eastl::vector<MeshVertex_T> inputVerticesTempBuffer;
	eastl::vector<uint32_t> inputIndicesTempBuffer;

	uint32_t totalInputVerticesSize = 0;
	uint32_t totalInputIndicesSize = 0;

	const MeshInfo* meshInfo = character.GetMeshInfo();
	
	uint32_t meshMaxCount = meshInfo->meshData->count;
	for (uint32_t i = 0; i < meshMaxCount; ++i)
	{
		totalInputVerticesSize += meshInfo->meshData->array[i].verticesCount;
		totalInputIndicesSize += meshInfo->meshData->array[i].indicesCount;
	}

	inputVerticesTempBuffer.reserve(totalInputVerticesSize);
	inputIndicesTempBuffer.reserve(totalInputIndicesSize);
	
	indicesBufferSize = totalInputIndicesSize;
	uint32_t vertexIndexOffset = 0;
	int num = 0;
	int cnt = 0;
	for (uint32_t i = 0; i < meshMaxCount; ++i)
	{
		for (uint32_t j = 0; j < meshInfo->meshData->array[i].verticesCount; ++j)
		{
			MeshVertex_T data = meshInfo->meshData->array[i].vertices[j];

			printf("v %d : %f %f %f %f uv : %f , %f \n", cnt++, data.position.x , data.position.y , data.position.z, data.position.w , data.uv.x , data.uv.y);
			
			inputVerticesTempBuffer.push_back(data);
		}
		

		for (uint32_t j = 0, size = meshInfo->meshData->array[i].indicesCount; j < size; ++j)
		{
			inputIndicesTempBuffer.push_back(vertexIndexOffset + meshInfo->meshData->array[i].indices[j]);
		}

		vertexIndexOffset += meshInfo->meshData->array[i].verticesCount;
	}

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(MeshVertex_T) * inputVerticesTempBuffer.size();
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * inputIndicesTempBuffer.size();
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA verticesData;
	verticesData.pSysMem = inputVerticesTempBuffer.data();

	D3D11_SUBRESOURCE_DATA indicesData; 
	indicesData.pSysMem =  inputIndicesTempBuffer.data();

	HRESULT hr = dev->CreateBuffer(&vertexBufferDesc, &verticesData, &verticesBufferHandle);
	if (!SUCCEEDED(hr))
	{
		assert(false && "create vertexBuffer fail");
	}
	hr = dev->CreateBuffer(&indexBufferDesc, &indicesData, &indicesBufferHandle);
	if (!SUCCEEDED(hr))
	{
		assert(false && "create indexBuffer fail");
	}

	//texture data init
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	textureSRVHandleArr = (ID3D11ShaderResourceView**)malloc(sizeof(ID3D11ShaderResourceView*));
	CreateSRVArrayFromTextureResource(dev ,character.GetTextureResource() , textureSRVHandleArr);
	dev->CreateSamplerState(&samplerDesc ,&samplerHandle);
	devCon->PSSetShaderResources(0 , 1 , textureSRVHandleArr);
	devCon->PSSetSamplers(0 , 1, &samplerHandle);

}

void CharacterRenderer::CBInit(ID3D11Device* dev)
{
	D3D11_BUFFER_DESC camBufferDesc = {};
	camBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 2;
	camBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	camBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	camBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	camBufferDesc.MiscFlags = 0;
	camBufferDesc.StructureByteStride = 0;

	dev->CreateBuffer(&camBufferDesc , nullptr , &pCamBuffer);


	D3D11_BUFFER_DESC modelBufferDesc = {};
	modelBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 1;
	modelBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	modelBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	modelBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	modelBufferDesc.MiscFlags = 0;
	modelBufferDesc.StructureByteStride = 0;

	dev->CreateBuffer(&modelBufferDesc, nullptr, &pModelBuffer);


	D3D11_BUFFER_DESC jointsBufferDesc = {};
	jointsBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4)* 128;
	jointsBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	jointsBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	jointsBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	jointsBufferDesc.MiscFlags = 0;
	jointsBufferDesc.StructureByteStride = 0;

	dev->CreateBuffer(&jointsBufferDesc, nullptr, &pJointsBuffer);


	D3D11_BUFFER_DESC modelNDCBufferDesc = {};
	modelNDCBufferDesc.ByteWidth = sizeof(DirectX::XMFLOAT4X4) * 1;
	modelNDCBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	modelNDCBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	modelNDCBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	modelNDCBufferDesc.MiscFlags = 0;
	modelNDCBufferDesc.StructureByteStride = 0;

	dev->CreateBuffer(&modelNDCBufferDesc, nullptr, &pModelNDCBuffer);
}

void CharacterRenderer::CBModelUpdate(ID3D11DeviceContext* devCon , DirectX::XMMATRIX& modelMat)
{
	D3D11_MAPPED_SUBRESOURCE mapped;

	devCon->Map(pModelBuffer , 0 , D3D11_MAP_WRITE_DISCARD ,  0 , &mapped);
	memcpy(mapped.pData , &modelMat , sizeof(DirectX::XMMATRIX));
	devCon->Unmap(pModelBuffer , 0);

	devCon->VSSetConstantBuffers(1 , 1 , &pModelBuffer);
}


void CharacterRenderer::CBCamUpdate(ID3D11DeviceContext* devCon, DirectX::XMMATRIX& viewMat, DirectX::XMMATRIX& projMat)
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	Cam_S cam = {
		viewMat,
		projMat
	};
	devCon->Map(pCamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData,&cam, sizeof(cam));
	devCon->Unmap(pCamBuffer, 0);

	devCon->VSSetConstantBuffers(0 , 1, &pCamBuffer);
}

void CharacterRenderer::CBJointsUpdate(ID3D11DeviceContext* devCon, DirectX::XMMATRIX* jointsPoseMatArr, uint32_t activeJointsCount)
{
	D3D11_MAPPED_SUBRESOURCE mapped;


	devCon->Map(pJointsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, jointsPoseMatArr, sizeof(DirectX::XMMATRIX) * activeJointsCount);
	devCon->Unmap(pJointsBuffer, 0);

	devCon->VSSetConstantBuffers(2, 1, &pJointsBuffer);

}

void CharacterRenderer::CBModelNDCUpdate(ID3D11DeviceContext* devCon, Character& character)
{

	D3D11_MAPPED_SUBRESOURCE mapped;
	const DirectX::XMFLOAT4X4* m = character.GetModelNDCMat();
	DirectX::XMMATRIX data = DirectX::XMLoadFloat4x4(m);

	devCon->Map(pModelNDCBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &data, sizeof(DirectX::XMMATRIX));
	devCon->Unmap(pModelNDCBuffer, 0);

	devCon->VSSetConstantBuffers(3, 1, &pModelNDCBuffer);

}

void CharacterRenderer::SetPipeLine(ID3D11DeviceContext* devCon)
{
	uint32_t stride = sizeof(MeshVertex_T);
	uint32_t offset = 0;

	devCon->VSSetShader(characterVertexShader, nullptr, 0);
	devCon->PSSetShader(characterPixelShader, nullptr, 0);
	devCon->IASetInputLayout(inputLayout);

	devCon->VSSetConstantBuffers(0, 1, &pCamBuffer);    
	devCon->VSSetConstantBuffers(1, 1, &pModelBuffer); 
	devCon->VSSetConstantBuffers(2, 1, &pJointsBuffer); 
	devCon->VSSetConstantBuffers(3, 1, &pModelNDCBuffer);
}

void CharacterRenderer::SetCharacter(ID3D11DeviceContext* devCon , Character& character)
{


	DirectX::XMMATRIX initModel =   {50.F , 0.F , 0.F , 0.F,
									 0.F ,50.F , 0.F , 0.F ,
									 0.F , 0.F , 50.F , 0.F ,
									  -376.000000F , 61.004238F ,  -608.926636F , 1.F };

	const DirectX::XMFLOAT4X4* joints = character.GetCurrentJointsMatrix(&jointsCount);
	const DirectX::XMFLOAT4X4* inverseJoints = character.GetInverseJoints(&jointsCount);
	jointsMatrixBuffer = new DirectX::XMMATRIX[jointsCount];

	for (int i = 0; i < jointsCount; ++i)
	{
		
		jointsMatrixBuffer[i] = DirectX::XMLoadFloat4x4(&inverseJoints[i]) * DirectX::XMLoadFloat4x4(&joints[i]);
		jointsMatrixBuffer[i] = DirectX::XMMatrixTranspose(jointsMatrixBuffer[i]);
		
	}//

	CBJointsUpdate(devCon, jointsMatrixBuffer, jointsCount);

	CBModelUpdate(devCon, initModel);

	CBModelNDCUpdate(devCon , character);
}

void CharacterRenderer::Draw(ID3D11DeviceContext* devCon)
{
	uint32_t stride = sizeof(MeshVertex_T);
	uint32_t offset = 0;

	devCon->PSSetShaderResources(0, 1, textureSRVHandleArr);
	devCon->PSSetSamplers(0, 1, &samplerHandle);

	devCon->IASetVertexBuffers(0, 1, &verticesBufferHandle, &stride, &offset);
	devCon->IASetIndexBuffer(indicesBufferHandle , DXGI_FORMAT_R32_UINT , 0);

	devCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	devCon->DrawIndexed(indicesBufferSize,0, 0);
	
}


void CreateSRVArrayFromTextureResource(ID3D11Device* dev,const TextureResourec_T* textureRrsData,ID3D11ShaderResourceView** textureSRV)
{
	uint32_t maxHeight = 0;
	uint32_t maxWidth = 0;
	
	for (uint32_t i = 0, size = textureRrsData->size; i < size; ++i)
	{
		const DirectX::Image* image = textureRrsData->Images[i].GetImages();
		if (image->width > maxWidth) maxWidth = image->width;
		if (image->height > maxHeight) maxHeight = image->height;

	}
	
	// 1. resize 일단 max 사이즈로 1이미지 에대해 1텍스처 이미지 생성
	/*
		1. 의 방식은 모든 이미지를 max이미지로 resize 하여 GPU 메모리 낭비가 큼
		but 1 회 Draw Call 을 통해 draw 하기 위해서는 D3D11 에서는 Texture2DArray 형태가 필요하고 이는 텍스처의 크기가 동일해야함

		현재 캐릭터 개발 이후 해당 resize 및 텍스처 부분 아래와 같이 최적화 필요 

		 2. UV 좌표를 변조하지 않고 이미지를 쪼갠 뒤에 삼각형을 전달 시 텍스처 인덱스의 범위 함께 전달해주는 방식
		 (1 이미지 n 개의 텍스처로 분할 전송)
		 예) 시작 인덱스 5번이라면 분할 사이즈 정보 4를 넘김 
		 즉 5 ,6 ,7, 8 인덱스를 참조할 수 있도록 만일 1개 이미지 4깨 분할 후 일렬로 정렬하여 전달시 
		 UV 의 u 가 0.5 보다 큰가 작은가 v가 0.5 보다 큰가 작은 가 로 index를 지정 
		 그리고 0.5 의 경계에 걸치를 부분은 텍스처가 서로 겹치부분 오버랩 하여 검은 선이 나타나는 버그 차단

		 위 방식을 통해 max 의 크기를 줄여 작은 이미지가 기존에 max 만큼 커져야 하는 메모리 낭비를 막으며 1 Draw Call 로 Draw 가능함

		 2. 방식은 실험적 , 1. 먼저 성공 후 2. 시도
	
	*/

	D3D11_SUBRESOURCE_DATA* subResourceData = (D3D11_SUBRESOURCE_DATA*)malloc(sizeof(D3D11_SUBRESOURCE_DATA) * textureRrsData->size);
	BYTE** imageArr = (BYTE**)malloc(sizeof(BYTE*) * textureRrsData->size );
	
	for (uint32_t i = 0, size = textureRrsData->size; i < size; ++i)
	{
		const DirectX::Image* srcImage = textureRrsData->Images[i].GetImages();
		imageArr[i] = (BYTE*)malloc(sizeof(BYTE) * maxHeight * maxWidth * 4);


		for (uint32_t h = 0; h < maxHeight; ++h)
		{
			uint32_t hIdx = (uint32_t)(((float)(h * srcImage->height) / maxHeight));
			uint32_t dstRowOffset = h * maxWidth * 4;
			uint32_t srcRowOffset = (uint32_t)(hIdx * srcImage->rowPitch ); 

			for (uint32_t w = 0; w < maxWidth; ++w)
			{
				
				uint32_t wIdx = (uint32_t)(((float)(w * srcImage->width ) / maxWidth));
				imageArr[i][dstRowOffset + w * 4 + 0] = srcImage->pixels[srcRowOffset + wIdx * 4 + 0];
				imageArr[i][dstRowOffset + w * 4 + 1] = srcImage->pixels[srcRowOffset + wIdx * 4 + 1];
				imageArr[i][dstRowOffset + w * 4 + 2] = srcImage->pixels[srcRowOffset + wIdx * 4 + 2];
				imageArr[i][dstRowOffset + w * 4 + 3] = srcImage->pixels[srcRowOffset + wIdx * 4 + 3];
			}
		}

		subResourceData[i].pSysMem = imageArr[i];
		subResourceData[i].SysMemPitch = maxWidth * 4;
		subResourceData[i].SysMemSlicePitch = 0;
	}


	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = maxWidth;
	textureDesc.Height = maxHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = textureRrsData->size;

	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;


	//texture2D 구조 생성
	ID3D11Texture2D* pTexture2D = nullptr;
	dev->CreateTexture2D(&textureDesc, subResourceData, &pTexture2D);

	//SRV 생성 DirectX 에서는 view단위로???
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.ArraySize = textureDesc.ArraySize;
	srvDesc.Texture2DArray.FirstArraySlice = 0;

	dev->CreateShaderResourceView(pTexture2D, &srvDesc , textureSRV);
	pTexture2D->Release();
	free(subResourceData);
	for (uint32_t i = 0, size = textureRrsData->size; i < size; ++i)
	{
		free(imageArr[i]);
	}
	free(imageArr);
}

