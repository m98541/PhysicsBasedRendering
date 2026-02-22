#include "MJ_D3D11_Character.h"

Character::Character()
{
	skeleton = nullptr;
	mesh = nullptr;
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

void update()
{

}



