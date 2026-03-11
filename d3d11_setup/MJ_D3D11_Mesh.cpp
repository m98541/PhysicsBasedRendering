#include "MJ_D3D11_CharacterResource.h"

using namespace DirectX;

MeshInfo::MeshInfo()
{
	this->meshData = nullptr;
}

MeshInfo::MeshInfo(MeshResource_T* meshData)
{
	
	this->meshData = meshData;
}

MeshInfo::~MeshInfo()
{

}

