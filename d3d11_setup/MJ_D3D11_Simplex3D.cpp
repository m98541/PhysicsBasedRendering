#include "MJ_D3D11_Simplex3D.h"
#include <EASTL/sort.h>

using namespace DirectX;
using namespace eastl;
using namespace HalfEdge;

Simpelx3D::Simpelx3D()
{
	point[0] = {0,};
	point[1] = { 0, };
	point[2] = { 0, };
	point[3] = { 0, };
}

Simpelx3D::~Simpelx3D()
{

}

void Simpelx3D::CreateSimplex3D(XMVECTOR* inVertexArray, unsigned int size)
{

	DirectX::XMVECTOR* tempVertArray = (DirectX::XMVECTOR*)malloc(sizeof(DirectX::XMVECTOR) * size);
	if (tempVertArray != NULL)
		memcpy(tempVertArray, inVertexArray, sizeof(DirectX::XMVECTOR) * size);
	else
		return;

	XMVECTOR axisMaxMin[6];

	for (int axis = 0; axis < 3; axis++)
	{
		eastl::sort(tempVertArray + 0, tempVertArray + size, [axis](const DirectX::XMVECTOR& lVert, const DirectX::XMVECTOR& rVert) {
			return lVert.m128_f32[axis] < rVert.m128_f32[axis];
			});

		axisMaxMin[axis] = tempVertArray[size - 1];
		axisMaxMin[axis + 3] = tempVertArray[0];
	}

	
	double maxLineLen = 0.F;

	for (int i = 0; i < 5; i++)
	{
		for (int j = i + 1; j < 6; j++)
		{
			double curLen = XMVector3Length(axisMaxMin[j] - axisMaxMin[i]).m128_f32[0];
			if (curLen > maxLineLen)
			{
				maxLineLen = curLen;
				this->point[0] = axisMaxMin[i];
				this->point[1] = axisMaxMin[j];
			}
		}
	}

	maxLineLen = 0.F;
	for (int i = 0; i < size; i++)
	{
		double curLen = XMVector3LinePointDistance(this->point[0], this->point[1], tempVertArray[i]).m128_f32[0];
		if (curLen > maxLineLen)
		{
			maxLineLen = curLen;
			this->point[2] = axisMaxMin[i];
		}
	}


	maxLineLen = 0.F;
	XMVECTOR simplexNormal = XMVector3Normalize(XMVector3Cross((this->point[1] - this->point[0]), (this->point[2] - this->point[0])));
	for (int i = 0; i < size; i++)
	{
		double curLen = XMVector3Dot(simplexNormal, tempVertArray[i]).m128_f32[0];
		if (curLen > maxLineLen)
		{
			maxLineLen = curLen;
			this->point[3] = axisMaxMin[i];
		}
	}


	//init simplex tetrahedron index buf
	unsigned int indexArr[12] = {
		0,2,1,
		0,3,2,
		0,1,3,
		1,2,3
	};

	CreateHESetFromVertexBuffer(this->point,4 ,indexArr ,12 , &this->simplexHESet);
	free(tempVertArray);
}

bool Simpelx3D::IsPointInSimplex(XMVECTOR point)
{
	if (
		XMVector3Dot(this->simplexHESet.faceSet[0].norm, point).m128_f32[0] <= 0.0 &&
		XMVector3Dot(this->simplexHESet.faceSet[1].norm, point).m128_f32[0] <= 0.0 &&
		XMVector3Dot(this->simplexHESet.faceSet[2].norm, point).m128_f32[0] <= 0.0 &&
		XMVector3Dot(this->simplexHESet.faceSet[3].norm, point).m128_f32[0] <= 0.0
		)
	{
		return true;
	}
	else
	{
		return false;
	}
}