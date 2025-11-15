#include "MJ_D3D11_Simplex3D.h"
#include <EASTL/sort.h>

using namespace DirectX;
using namespace eastl;
using namespace HalfEdge;

Simpelx3D::Simpelx3D()
{
	faceCount = 0;
	tempVertArray = nullptr; 
}



Simpelx3D::Simpelx3D(XMVECTOR* inVertexArray, unsigned int size)
{
	faceCount = 0;
	tempVertArray = (DirectX::XMVECTOR*)malloc(sizeof(DirectX::XMVECTOR) * size);
	CreateSimplex3D(inVertexArray, size);
}

Simpelx3D::~Simpelx3D()
{
	free(tempVertArray);
}

void Simpelx3D::CreateSimplex3D(XMVECTOR* inVertexArray, unsigned int size)
{
	

	

	if (tempVertArray != NULL)
		memcpy(tempVertArray, inVertexArray, sizeof(DirectX::XMVECTOR) * size);
	else
		return;

	XMVECTOR axisMaxMin[6];
	//init simplex tetrahedron vertex buf
	DirectX::XMVECTOR point[4];
	//init simplex tetrahedron index buf
	unsigned int indexArr[12] = {
		0,2,1,
		0,3,2,
		0,1,3,
		1,2,3
	};
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
				point[0] = axisMaxMin[i];
				point[1] = axisMaxMin[j];
			}
		}
	}

	maxLineLen = 0.F;
	for (int i = 0; i < size; i++)
	{
		double curLen = XMVector3LinePointDistance(point[0], point[1], tempVertArray[i]).m128_f32[0];
		if (curLen > maxLineLen)
		{
			maxLineLen = curLen;
			point[2] = tempVertArray[i];
		}
	}


	maxLineLen = 0.F;
	XMVECTOR simplexNormal = XMVector3Normalize(XMVector3Cross((point[1] - point[0]), (point[2] - point[0])));
	for (int i = 0; i < size; i++)
	{
		double curLen = XMVector3Dot(simplexNormal, tempVertArray[i] - point[0] ).m128_f32[0];
		if (abs(curLen) > maxLineLen)
		{
			maxLineLen = abs(curLen);
			point[3] = tempVertArray[i];
		}
	}


	double testLen = XMVector3Dot(simplexNormal, point[3] - point[0]).m128_f32[0];
	if (testLen < 0)
	{
		

		indexArr[0] = 0; indexArr[1] = 2; indexArr[2] = 1; // (0, 2, 1)
		indexArr[3] = 0; indexArr[4] = 3; indexArr[5] = 2; // (0, 3, 2)
		indexArr[6] = 0; indexArr[7] = 1; indexArr[8] = 3; // (0, 1, 3)
		indexArr[9] = 1; indexArr[10] = 2; indexArr[11] = 3; // (1, 2, 3)
	}
	CreateHESetFromVertexBuffer(point,4 ,indexArr ,12 , &this->simplexHESet);
	this->faceCount = 4;
	
}

bool Simpelx3D::IsPointInSimplex(XMVECTOR point ,int* outFaceId)
{
	for (int i = 0; i < this->faceCount; i++)
	{
		HalfEdge::HE_FACE_T* face = &this->simplexHESet.faceSet[i];
		XMVECTOR p0 = this->simplexHESet.edgeSet[i * 3].originVert->pos;
		if (XMVector3Dot(face->norm, point - p0 ).m128_f32[0] > 0) 
		{
			*outFaceId = i;
			return false;
		}
	}
	return true;
}