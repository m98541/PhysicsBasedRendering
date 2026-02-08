#include "MJ_D3D11_Mesh.h"

using namespace DirectX;

Mesh::Mesh()
{
	this->meshData = nullptr;
}

Mesh::Mesh(MeshResource_T* meshData)
{
	this->meshData = meshData;
}

Mesh::~Mesh()
{

}

void Mesh::D3D11Load()
{

}

void Mesh::D3D11Draw()
{

}