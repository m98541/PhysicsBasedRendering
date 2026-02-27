#include "MJ_D3D11_Character.h"

Character::Character()
{
	skeleton = nullptr;
	mesh = nullptr;

	resourceData = nullptr;

	pos = { 0.F , 0.F , 0.F ,1.F };//{ -376.000000F , 61.004238F ,  -608.926636F , 1.F };
	direction = { 0.F , 0.F , 0.F , 1.F };
	scale = {0.5F ,0.5F ,0.5F ,1.F};
}

Character::~Character()
{

}

Character::Character(CharacterResource* characterResource)
{
	Character::LoadResource(characterResource);
}

void Character::LoadResource(CharacterResource* characterResource)
{
	skeleton = new Skeleton(&characterResource->skeletonResource);
	mesh = new MeshInfo(&characterResource->meshResource);
	resourceData = characterResource;
	modelNDCMat = &characterResource->modelNDCMat;
}

const MeshInfo* Character::GetMeshInfo()
const 
{
	return mesh;
}

const Skeleton* Character::GetSkeleton()
const
{
	return skeleton;
}

const TextureResourec_T* Character::GetTextureResource()
const
{
	return &resourceData->TextureResource;
}

DirectX::XMMATRIX Character::GetModelMatrix()
const
{
	DirectX::XMMATRIX modelMat = DirectX::XMMatrixAffineTransformation(
		DirectX::XMLoadFloat4(&scale),
		DirectX::g_XMZero,
		DirectX::XMLoadFloat4(&direction),
		DirectX::XMLoadFloat4(&pos)
	);
	return modelMat;
}


const DirectX::XMFLOAT4X4* Character::GetModelNDCMat()
const
{
	return modelNDCMat;
}

const DirectX::XMFLOAT4X4* Character::GetCurrentJointsMatrix( uint32_t* count)
{
	*count = this->skeleton->GetGlobalJointsCount();
	const DirectX::XMFLOAT4X4* matArr = this->skeleton->GetGlobalJoints();
	return matArr;

}
const DirectX::XMFLOAT4X4* Character::GetInverseJoints(uint32_t* count)
{
	*count = this->skeleton->GetGlobalJointsCount();
	const DirectX::XMFLOAT4X4* matArr = this->skeleton->GetInverseJoints();
	return matArr;
}

void update()
{

}



