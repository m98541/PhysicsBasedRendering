#ifndef MJ_D3D11_CHARACTER_RESOURCE_H
#define MJ_D3D11_CHARACTER_RESOURCE_H
#include <DirectXTex.h>
#include "MJ_D3D11_Skeleton.h"
#include "MJ_D3D11_Mesh.h"
#include "MJ_D3D11_Animation.h"

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


typedef struct AnimationClipResourec_S
{
	std::string name;
	float totalTime;
	uint32_t count;// 당연히 현재 조인트와 같아야 하지만 잘못된 에니메이션일 수 있기때문에 검증용
	AnimationJoint_T* AnimationJoint;
}AnimationClipResourec_T;

typedef struct AnimationResource_S
{
	//하나의 에니메이션 리소스에서는 여러 클립의 리소스가 존재함
	uint32_t count;
	AnimationClipResourec_T* animationClip;
	
}AnimationResource_T;

class CharacterResource
{
	public:
		AnimationResource_T animationResource;
		MeshResource_T meshResource;
		SkeletonResource_T skeletonResource;
		TextureResourec_T TextureResource;
		DirectX::XMFLOAT4X4 modelNDCMat;
};
#endif // !MJ_D3D11_CHARACTERRE_SOURCE_H
