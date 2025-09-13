#pragma once
#ifndef MJ_D3D11_SIMPLEX3D_H
#define MJ_D3D11_SIMPLEX3D_H
#include <DirectXMath.h>

class Simpelx3D
{
public:
	Simpelx3D();
	~Simpelx3D();

	void CreateSimplex3D(DirectX::XMVECTOR* inVertexArray, unsigned int size);
	DirectX::XMVECTOR point[4];
	DirectX::XMVECTOR norm[4];
	bool IsPointInSimplex(DirectX::XMVECTOR point);

};

#endif // !MJ_D3D11_SIMPLEX3D_H
