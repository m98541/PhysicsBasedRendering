#include "MJ_D3D11_GJK.h"

using namespace DirectX;

gjkSimplex::gjkSimplex()
{
	points[0] = { 0, };
	points[1] = { 0, };
	points[2] = { 0, };
	points[3] = { 0, };
	level = 0;
}

gjkSimplex::~gjkSimplex()
{
	
}

void gjkSimplex::addPoint(XMVECTOR point)
{
	points[level] = point;
	level++;
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point)
{
	level = 1;
	points[0] = point;
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1)
{
	level = 2;
	points[0] = point0;
	points[1] = point1;
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1, DirectX::XMVECTOR point2)
{
	level = 3;
	points[0] = point0;
	points[1] = point1;
	points[2] = point2;
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1, DirectX::XMVECTOR point2 , DirectX::XMVECTOR point3)
{
	level = 4;
	points[0] = point0;
	points[1] = point1;
	points[2] = point2;
	points[3] = point3;
}

const XMVECTOR ORIGIN = {0,0,0,0};

bool HandleSimplexPoint(gjkSimplex& simplex, DirectX::XMVECTOR& direction);
bool HandleSimplexLine(gjkSimplex& simplex, DirectX::XMVECTOR& direction);
bool HandleSimplexTriangle(gjkSimplex& simplex, DirectX::XMVECTOR& direction);
bool HandleSimplexTetrahedron(gjkSimplex& simplex, DirectX::XMVECTOR& direction);

bool selectSimplexHandle(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	switch (simplex.level)
	{
	case 1:
		return HandleSimplexPoint(simplex, direction);
	case 2:
		return HandleSimplexLine(simplex, direction);
	case 3:
		return HandleSimplexTriangle(simplex, direction);
	case 4:
		return HandleSimplexTetrahedron(simplex, direction);
	default:
		break;
	}


}


bool gjkCollisionCheck(ConvexHull* convexA, DirectX::XMMATRIX matTRS_A, ConvexHull* convexB, DirectX::XMMATRIX matTRS_B, gjkSimplex& simplex)
{
	XMVECTOR posA = {0 , 0 , 0 , 1.F}; 

	posA =XMVector3Transform(posA , matTRS_A);

	XMVECTOR posB = { 0 , 0 , 0 , 1.F };
	posB = XMVector3Transform(posB, matTRS_B);

	XMVECTOR direction = XMVector3Normalize(posB - posA);

	simplex.addPoint(convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A));
	
	XMVECTOR farVector;

	while (true)
	{

		farVector = convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A);
		if (XMVector3Dot(farVector, direction).m128_f32[0] >= 0)
		{
			return false;
		}

		if (selectSimplexHandle(simplex, direction))
		{// 0점을 찾아낸 경우
			
			return true;
		}
		else
		{	//못찾은 경우 simplex 확장
			simplex.addPoint(convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A));
		}

	}
	
	


}

bool HandleSimplexPoint(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	XMVECTOR point = simplex.points[0];

	if (XMVector3Equal(point, ORIGIN))
	{
		return true;
	}

	direction = XMVector3Normalize(ORIGIN - point);
	return false;
}

bool HandleSimplexLine(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	XMVECTOR pointA = simplex.points[0];
	XMVECTOR pointB = simplex.points[1];
	XMVECTOR lineBA = pointB - pointA;
	XMVECTOR lineAO = ORIGIN - pointA;

	direction = XMVector3Cross(XMVector3Cross(lineBA , lineAO) , lineBA);

	if (XMVector3LengthSq(direction).m128_f32[0] < 1e-6f)
	{
		XMVECTOR ratio = XMVector3Dot(lineAO, lineBA) / XMVector3LengthSq(lineBA);

		if (ratio.m128_f32[0] >= 0 && ratio.m128_f32[0] <= 1.F)
		{
			return true; 
		}

		if (ratio.m128_f32[0] < 0.F)
		{
			simplex.SetPoints(pointA);
			direction = lineAO;
			return false;
		}

		if (ratio.m128_f32[0] > 1.F)
		{
			simplex.SetPoints(pointB);
			direction = ORIGIN - pointB;
			return false;
		}

	}

	direction = XMVector3Normalize(direction);
	return false;
}

bool HandleSimplexTriangle(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	XMVECTOR pointA = simplex.points[0];
	XMVECTOR pointB = simplex.points[1];
	XMVECTOR pointC = simplex.points[2];

	XMVECTOR lineAB = pointB - pointA;
	XMVECTOR lineAC = pointC - pointA;
	XMVECTOR lineAO = ORIGIN - pointA;

	XMVECTOR triNormal = XMVector3Cross(lineAB , lineAC);
	if (XMVector3Dot(triNormal, lineAO).m128_f32[0] > 0)
	{
		direction = XMVector3Normalize(triNormal);
	}
	else
	{
		direction = -XMVector3Normalize(triNormal);
	}

	return false;

}

bool HandleSimplexTetrahedron(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	/*
	0,2,1,
	0,3,2,
	0,1,3,
	1,2,3
	*/
	XMVECTOR point0 = simplex.points[0];
	XMVECTOR point1 = simplex.points[1];
	XMVECTOR point2 = simplex.points[2];
	XMVECTOR point3 = simplex.points[3];

	XMVECTOR face[4][3] = {
		{point0 ,point2 ,point1 },
		{point0 ,point3 ,point2 },
		{point0 ,point1 ,point3 },
		{point1 ,point2 ,point3 },
	};

	memcpy(simplex.faces , face , sizeof(XMVECTOR)*12);


	for (int i = 0; i < 4; i++)
	{
		XMVECTOR faceNorm = XMVector3Cross(face[i][1] - face[i][0], face[i][2] - face[i][0]);
		

		XMVECTOR p0 = face[i][0];
		if (XMVector3Dot(faceNorm, p0).m128_f32[0] < 0.F)
		{
			faceNorm = -faceNorm;
		}

		if (XMVector3Dot(faceNorm , ORIGIN - p0).m128_f32[0] > 0.F)
		{
			direction = XMVector3Normalize(faceNorm);
			simplex.SetPoints(face[i][0], face[i][1], face[i][2]);

			return false;
		}
	}
	return true;

}