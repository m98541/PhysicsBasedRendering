#pragma once
#ifndef MJ_D3D11_SIMPLEX3D_H
#define MJ_D3D11_SIMPLEX3D_H
#include <DirectXMath.h>
#include "MJ_D3D11_HalfEdge.h"

class Simpelx3D
{
public:
	Simpelx3D();
	Simpelx3D(DirectX::XMVECTOR* inVertexArray, unsigned int size);

	~Simpelx3D();

	
	HalfEdge::HE_SET_T simplexHESet;
	
	int faceCount;
	void CreateSimplex3D(DirectX::XMVECTOR* inVertexArray, unsigned int size);
	bool IsPointInSimplex(DirectX::XMVECTOR point, int* outFaceId);
private:
	DirectX::XMVECTOR* tempVertArray;
};

#endif // !MJ_D3D11_SIMPLEX3D_H
