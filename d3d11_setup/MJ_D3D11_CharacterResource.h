#ifndef MJ_D3D11_CHARACTER_RESOURCE_H
#define MJ_D3D11_CHARACTER_RESOURCE_H
#include <DirectXTex.h>
#include "MJ_D3D11_Skeleton.h"
#include "MJ_D3D11_Mesh.h"

typedef struct SkeletonResource_S
{
	unsigned int count;
	Joint_T* array;
}SkeletonResource_T;

typedef struct MeshResource_S
{
	unsigned int count;
	Mesh_T* array;
}MeshResource_T;

typedef struct ImageData_S
{
	size_t size;
	uint8_t* Data;
}ImageData_T;

// 이후 material 데이터 로 확장 후 texture는 material 정보에 포함 될 예정
typedef struct TextureResource_S
{
	size_t size;
	DirectX::ScratchImage* Images;
}TextureResourec_T;

class CharacterResource
{
	public:
		MeshResource_T meshResource;
		SkeletonResource_T skeletonResource;
		TextureResourec_T TextureResource;
};
#endif // !MJ_D3D11_CHARACTERRE_SOURCE_H
