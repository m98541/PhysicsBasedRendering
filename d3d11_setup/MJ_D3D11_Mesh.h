#ifndef MJ_D3D11_MESH_H
#define MJ_D3D11_MESH_H
#include <DirectXMath.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dtypes.h>


typedef struct MeshVertex_S
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 normal;
	DirectX::XMFLOAT2 uv;

}MeshVertex_T;


typedef struct Mesh_S
{
	unsigned int* indices;
	MeshVertex_T* vertices;
	int indicesCount;
	int verticesCount;
	int jointId;

}Mesh_T;

typedef struct MeshResource_S
{
	unsigned int count;
	Mesh_T* array;
}MeshResource_T;


class Mesh {
	
	public:
		Mesh();
		Mesh(MeshResource_T* meshData);
		~Mesh();

		void D3D11Load();
		void D3D11Draw();

	private:
		ID3D11Device* dev;
		ID3D11DeviceContext* devCon;
		ID3D10Blob* vsShader;
		D3D11_INPUT_ELEMENT_DESC* inputElement;
		int inputElementSize;

		MeshResource_T* meshData;
};
#endif // !MJ_D3D11_MESH_H
