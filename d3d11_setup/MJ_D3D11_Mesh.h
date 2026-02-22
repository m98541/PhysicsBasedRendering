#ifndef MJ_D3D11_MESH_H
#define MJ_D3D11_MESH_H
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dtypes.h>

typedef struct MeshResource_S MeshResource_T;

typedef struct MeshVertex_S
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 uv;
	uint32_t textureId;// 이후 material ID 로 교체 예정 material 정보 내에 textureID 존재
	uint32_t joints[4];
	float weights[4];
}MeshVertex_T;


typedef struct Mesh_S
{
	unsigned int* indices;
	MeshVertex_T* vertices;
	int indicesCount;
	int verticesCount;
}Mesh_T;

class MeshInfo {
	
	public:
		MeshInfo();
		MeshInfo(MeshResource_T* meshData); 
		~MeshInfo();

		D3D11_INPUT_ELEMENT_DESC* inputElement;
		int inputElementSize;
		MeshResource_T* meshData;

};
#endif // !MJ_D3D11_MESH_H
