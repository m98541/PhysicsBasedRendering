#include "MJ_D3D11_OBJLoader.h"

#include <stdio.h>

void CreateSRVFromBMPFile(ID3D11Device* device, const char* fileName, UINT bmp_format, ID3D11ShaderResourceView** Texture_SRV);
void CreateSRVArrayFromBMPFile(ID3D11Device* device, OBJFILE_BUFFER_T* objFileBuffer, UINT bmp_format, ID3D11ShaderResourceView** Texture_SRV);

bool MJD3D11LoadOBJ(ID3D11Device* Dev,ID3D11DeviceContext* DevCon, ID3D10Blob* vsShader,  MJD3D11OBJ_HANDLE_t** ObjHandle, const char* FileName)
{


	D3D11_BUFFER_DESC vertexBufferDesc = {};
	memset(&vertexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));
	D3D11_INPUT_ELEMENT_DESC inputElementDesc = {};
	memset(&inputElementDesc, 0, sizeof(D3D11_INPUT_ELEMENT_DESC));
	OBJFILE_DESC_T objFileDesc = {};
	memset(&objFileDesc, 0, sizeof(objFileDesc));

	OBJFILE_BUFFER_T* objFileBuffer = nullptr;

	*ObjHandle = (MJD3D11OBJ_HANDLE_t*)malloc(sizeof(MJD3D11OBJ_HANDLE_t));

	if (!ParseOBJFile(&objFileDesc , FileName))
	{
		printf("file access fail \n");
		return 0;
	}
	else 
	{
		printf("file parse success \n");

	}

	if (!ReadOBJFile(&objFileDesc, &objFileBuffer))
	{
		printf("file read fail \n");
	
		return 0;
	}
	else
		printf("file read success \n");
	
	

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc , sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	


	//일단 텍스처 bmp로 고정...결과는 봐야지..
	VERTEX_T* tempVertexBuffer = (VERTEX_T*)malloc(sizeof(VERTEX_T) * objFileBuffer->objectBufferLen);
	


	(*ObjHandle)->textureResourceViewHandleArr = (ID3D11ShaderResourceView**)malloc(sizeof(ID3D11ShaderResourceView*) * 1);
	// resource ready
	CreateSRVArrayFromBMPFile(Dev,objFileBuffer,BMP_FORMAT_BGR, (*ObjHandle)->textureResourceViewHandleArr);
	Dev->CreateSamplerState(&samplerDesc, &(*ObjHandle)->samplerHandle);
	DevCon->PSSetShaderResources(0,1, (*ObjHandle)->textureResourceViewHandleArr);
	DevCon->PSSetSamplers(0, 1, &(*ObjHandle)->samplerHandle);




	for (int i = 0; tempVertexBuffer != NULL && i < objFileBuffer->objectBufferLen; i++)
	{
		memcpy(&tempVertexBuffer[i].pos, objFileBuffer->objectBuffer + i * 6, sizeof(float) * 4);
		memcpy(&tempVertexBuffer[i].tex, objFileBuffer->objectBuffer + i * 6 + 4, sizeof(float) * 2);

		// norm 과 vertex 일치
		memcpy(&tempVertexBuffer[i].norm, objFileBuffer->objectBuffer + i * 6, sizeof(float) * 4);
		tempVertexBuffer[i].textureIdx = objFileBuffer->mtlBuffer[objFileBuffer->materialIdxBuffer[i]].textureIdx;
		

	}

	(*ObjHandle)->vertexCount = objFileBuffer->objectBufferLen;

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VERTEX_T) * objFileBuffer->objectBufferLen;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	Dev->CreateBuffer(&vertexBufferDesc, NULL , &(*ObjHandle)->vertexBufferHandle);

	D3D11_MAPPED_SUBRESOURCE mappingSrc;
	DevCon->Map((*ObjHandle)->vertexBufferHandle , NULL , D3D11_MAP_WRITE_DISCARD , NULL ,&mappingSrc);
	memcpy(mappingSrc.pData, tempVertexBuffer, sizeof(VERTEX_T) * objFileBuffer->objectBufferLen);
	DevCon->Unmap((*ObjHandle)->vertexBufferHandle , NULL);

	D3D11_INPUT_ELEMENT_DESC inputElement[4] = {
		//0바이트 부터 
		{"POSITION" , 0 , DXGI_FORMAT_R32G32B32A32_FLOAT, 0 , 0  , D3D11_INPUT_PER_VERTEX_DATA , 0},
		//12바이트 부터
		{"TEXCOORD" , 0 , DXGI_FORMAT_R32G32_FLOAT, 0 , 16 , D3D11_INPUT_PER_VERTEX_DATA , 0},

		{"NORM" , 0 , DXGI_FORMAT_R32G32B32A32_FLOAT, 0 , 24 , D3D11_INPUT_PER_VERTEX_DATA , 0},

		{"TEXIDX" , 0 ,DXGI_FORMAT_R32_UINT , 0 , 40 , D3D11_INPUT_PER_VERTEX_DATA , 0}

	};
	Dev->CreateInputLayout(inputElement, 4, vsShader->GetBufferPointer(), vsShader->GetBufferSize(), &(*ObjHandle)->inputLayout);
	DevCon->IASetInputLayout((*ObjHandle)->inputLayout);



	(*ObjHandle)->groupSet = objFileBuffer->groupSet;
	(*ObjHandle)->groupSetLen = objFileBuffer->groupSetLen;
	(*ObjHandle)->polygonSet = objFileBuffer->polygonSet;
	(*ObjHandle)->polygonSetLen = objFileBuffer->polygonSetLen;
	free(tempVertexBuffer);
	ReleaseOBJBuffer(&objFileBuffer);
	return 1;
}
int start = 1;

bool MJD3D11DrawOBJ(ID3D11DeviceContext* DevCon, MJD3D11OBJ_HANDLE_t* ObjHandle)
{
	UINT stride = sizeof(VERTEX_T);
	UINT offset = 0;

	DevCon->IASetVertexBuffers(0 ,1 ,&ObjHandle->vertexBufferHandle,&stride,&offset);
	DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	int len = ObjHandle->polygonSetLen;


	for (int i = 0; i < len; i++)
	{
		if (start)
		{
			//printf("draw polygon %d, offset : %d count : %d \n", i, ObjHandle->polygonSet[i].polygonOffset, ObjHandle->polygonSet[i].vertexCount);
			
		}
	
			DevCon->Draw(ObjHandle->polygonSet[i].vertexCount , ObjHandle->polygonSet[i].polygonOffset);
		
	}
	
	
	start = 0;
	

	return 1;

}

void CreateSRVFromBMPFile(ID3D11Device* device, const char* fileName, UINT bmp_format, ID3D11ShaderResourceView** Texture_SRV)
{
	//텍스처 이미지 로드
	BMPFILE textureImage = {};
	LoadBmpFile(fileName, &textureImage, bmp_format);
	BYTE* image_32bit = (BYTE*)malloc(textureImage.width * textureImage.height * 4);

	for (int i = 0; i < textureImage.height; i++)
	{
		int srcRowOffset = i * (textureImage.width * textureImage.format + textureImage.widthPadding);
		int dstRowOffset = i * textureImage.width * 4;

		for (int j = 0; j < textureImage.width; j++)
		{
			image_32bit[dstRowOffset + j * 4 + 0] = textureImage.data[srcRowOffset + j * textureImage.format + 0];
			image_32bit[dstRowOffset + j * 4 + 1] = textureImage.data[srcRowOffset + j * textureImage.format + 1];
			image_32bit[dstRowOffset + j * 4 + 2] = textureImage.data[srcRowOffset + j * textureImage.format + 2];
			image_32bit[dstRowOffset + j * 4 + 3] = (bmp_format == BMP_FORMAT_BGR) ? 255 : textureImage.data[srcRowOffset + j * textureImage.format + 3];
		}

	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = textureImage.width;
	textureDesc.Height = textureImage.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;

	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	//리소스 구조체 선언
	D3D11_SUBRESOURCE_DATA source_data;
	source_data.pSysMem = image_32bit;
	//BMPFILE의 format은 한셀의 바이트 수 와 동일
	source_data.SysMemPitch = textureImage.width * BMP_FORMAT_BGRA;
	source_data.SysMemSlicePitch = 0;//3D 텍스처에서의 1장의 크기 2D의 경우 0

	//texture2D 구조 생성
	ID3D11Texture2D* pTexture2D = nullptr;
	device->CreateTexture2D(&textureDesc, &source_data, &pTexture2D);

	//SRV 생성 DirectX 에서는 view단위로???
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = textureDesc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(pTexture2D, &srv_desc, Texture_SRV);
	pTexture2D->Release();
	free(image_32bit);

}


void CreateSRVArrayFromBMPFile(ID3D11Device* device, OBJFILE_BUFFER_T* objFileBuffer, UINT bmp_format, ID3D11ShaderResourceView** Texture_SRV)
{
	//텍스처 이미지 로드
	//텍스처 이미지 로드
	BMPFILE* textureImage = (BMPFILE*)malloc(sizeof(BMPFILE) * objFileBuffer->textureImageNameLen);
	int maxHeight = 0;
	int maxWidth = 0;

	for (int i = 0; i < objFileBuffer->textureImageNameLen; i++)
	{
		LoadBmpFile(objFileBuffer->textureImageNameArr[i], textureImage + i, bmp_format);
		if (textureImage[i].width > maxWidth) maxWidth = textureImage[i].width;
		if (textureImage[i].height > maxHeight) maxHeight = textureImage[i].height;
	}



	D3D11_SUBRESOURCE_DATA* sourceArr_data = (D3D11_SUBRESOURCE_DATA*)malloc(sizeof(D3D11_SUBRESOURCE_DATA) * objFileBuffer->textureImageNameLen);
	BYTE** imageArr_32bit = (BYTE**)malloc(sizeof(BYTE*) * objFileBuffer->textureImageNameLen);

	for (int i = 0; i < objFileBuffer->textureImageNameLen; i++)
	{
		
		imageArr_32bit[i] = (BYTE*)malloc( maxWidth * maxHeight * 4);
		for (int k = 0; k < maxHeight; k++)
		{
			int h_idx = (int)(((float)(k*textureImage[i].height) / maxHeight) );
			int srcRowOffset = h_idx * (textureImage[i].width * textureImage[i].format + textureImage[i].widthPadding);
			int dstRowOffset = k * maxWidth * 4;

			for (int j = 0; j < maxWidth; j++)
			{
					int w_idx = (int)(((float)(j*textureImage[i].width) / maxWidth));
					imageArr_32bit[i][dstRowOffset + j * 4 + 0] = textureImage[i].data[srcRowOffset + w_idx * textureImage[i].format + 0];
					imageArr_32bit[i][dstRowOffset + j * 4 + 1] = textureImage[i].data[srcRowOffset + w_idx * textureImage[i].format + 1];
					imageArr_32bit[i][dstRowOffset + j * 4 + 2] = textureImage[i].data[srcRowOffset + w_idx * textureImage[i].format + 2];
					imageArr_32bit[i][dstRowOffset + j * 4 + 3] = (bmp_format == BMP_FORMAT_BGR) ? 255 : textureImage[i].data[srcRowOffset + w_idx * textureImage[i].format + 3];
				
			}


		}
		sourceArr_data[i].pSysMem = imageArr_32bit[i];
		//BMPFILE의 format은 한셀의 바이트 수 와 동일
		sourceArr_data[i].SysMemPitch = maxWidth * BMP_FORMAT_BGRA;
		sourceArr_data[i].SysMemSlicePitch = 0;//3D 텍스처에서의 1장의 크기 2D의 경우 0
	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = maxWidth;
	textureDesc.Height = maxHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = objFileBuffer->textureImageNameLen;

	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	//texture2D 구조 생성
	ID3D11Texture2D* pTexture2D = nullptr;
	device->CreateTexture2D(&textureDesc, sourceArr_data, &pTexture2D);
	
	//SRV 생성 DirectX 에서는 view단위로???
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = textureDesc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	srv_desc.Texture2DArray.MostDetailedMip = 0;
	srv_desc.Texture2DArray.MipLevels = 1;
	srv_desc.Texture2DArray.ArraySize = textureDesc.ArraySize;
	srv_desc.Texture2DArray.FirstArraySlice = 0;

	device->CreateShaderResourceView(pTexture2D, &srv_desc, Texture_SRV);
	pTexture2D->Release();
	free(textureImage);
	for (int i = 0; i < objFileBuffer->textureImageNameLen; i++) {
		free(imageArr_32bit[i]);
	}
	free(imageArr_32bit);
	free(sourceArr_data);
}

