#include "MJ_D3D11_Character.h"

Character::Character()
{
	skeleton = nullptr;
	mesh = nullptr;




	resourceData = nullptr;


	headCam = new BasicCam();

	pos = { -376.000000F , 61.004238F ,  -608.926636F , 1.F };
	direction = { 0.F , 0.F , -1.F , 1.F };
	scale = { 50.F ,50.F ,50.F ,1.F };
}

Character::~Character()
{
	headCam->~BasicCam();
}

Character::Character(CharacterResource* characterResource)
{
	headCam = new BasicCam();
	pos = { -376.000000F , 61.004238F ,  -608.926636F , 1.F };
	direction = { 0.F , 0.F , -1.F , 1.F };
	scale = { 50.F ,50.F ,50.F ,1.F };
	Character::LoadResource(characterResource);
}

void Character::LoadResource(CharacterResource* characterResource)
{
	skeleton = new Skeleton(&characterResource->skeletonResource);
	mesh = new MeshInfo(&characterResource->meshResource);
	resourceData = characterResource;
	modelNDCMat = &characterResource->modelNDCMat;
	animationManager = new AnimationManager(skeleton , &characterResource->animationResource);


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
	DirectX::XMVECTOR dirVec = DirectX::XMVectorSet(0.F, 0.F, -1.F, 0.F);
	DirectX::XMVECTOR upVec = DirectX::XMVectorSet(0.F, 1.F, 0.F, 0.F);


	DirectX::XMMATRIX rotMat = DirectX::XMMatrixLookToLH(DirectX::g_XMZero, dirVec, upVec);
	DirectX::XMMATRIX worldRotMat = DirectX::XMMatrixInverse(nullptr, rotMat);
	DirectX::XMVECTOR quat = DirectX::XMQuaternionRotationMatrix(worldRotMat);

	DirectX::XMMATRIX modelMat = DirectX::XMMatrixAffineTransformation(
		DirectX::XMLoadFloat4(&(this->scale)),
		DirectX::g_XMZero,
		quat, // 변환된 쿼터니언 사용
		DirectX::XMLoadFloat4(&(this->pos))
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

void Character::updateAnimation(uint32_t animationId, float deltaT)
{
	animationManager->SelectAnimation(animationId);
	animationManager->NextTimePose(deltaT);
}

void Character::updateHeadCam(BasicCam* outCamData)
{
	DirectX::XMVECTOR camPos = { 0.F , 0.F , 0.F , 1.F };
	DirectX::XMVECTOR camAt = {0.F , 0.F , -1.F , 1.F};
	DirectX::XMVECTOR camUp = { 0.F , 1.F , 0.F , 0.F };

	DirectX::XMMATRIX chrModelMat = this->GetModelMatrix();
	DirectX::XMFLOAT4X4 headJointFLoat4X4 = this->skeleton->GetGlobalHeadJointPoseMat();
	DirectX::XMMATRIX chrHeadJointMat = DirectX::XMLoadFloat4x4(&headJointFLoat4X4);
	DirectX::XMMATRIX ndcModelMat = DirectX::XMLoadFloat4x4(this->modelNDCMat);
	camPos = DirectX::XMVector3Transform(camPos ,  chrHeadJointMat * ndcModelMat * chrModelMat);
	camAt = DirectX::XMVector3Transform(camAt, chrHeadJointMat * ndcModelMat * chrModelMat);

	outCamData->SetPosV4f(camPos);

	outCamData->SetAtV4f(camAt);

	outCamData->SetUpV4f(camUp);



}



