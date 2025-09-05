#ifndef MJ_D3D11_OBJLOADER_H
#define MJ_D3D11_OBJLOADER_H

#include <windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dtypes.h>
#include "OBJFileIO.h"
#include "BmpFileIO.h"
#include "VertexType.h"

typedef struct MJD3D11OBJ_HANDLE
{
	ID3D11Buffer* vertexBufferHandle;
	int vertexCount;
	ID3D11ShaderResourceView** textureResourceViewHandleArr;
	ID3D11SamplerState* samplerHandle;
	ID3D11InputLayout* inputLayout;
	TRI_POLYGON_GROUP_T* groupSet;
	unsigned int groupSetLen;

	TRI_POLYGON_T* polygonSet;
	unsigned int polygonSetLen;

}MJD3D11OBJ_HANDLE_t;


bool MJD3D11LoadOBJ(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, ID3D10Blob* vsShader, MJD3D11OBJ_HANDLE_t** ObjHandle, const char* FileName);

// 일단은 통으로 뽑기 차후 그룹단위 
bool MJD3D11DrawOBJ(ID3D11DeviceContext* DevCon, MJD3D11OBJ_HANDLE_t* ObjHandle);
#endif // !MJ_D3D11_OBJLOADER_H
