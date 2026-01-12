#include "MJ_D3D11_GJK.h"

#define EPSILON 1e-5f
using namespace DirectX;

gjkSimplex::gjkSimplex()
{
	points[0] = XMFLOAT4(0.F ,0.F , 0.F , 0.F);
	points[1] = XMFLOAT4(0.F ,0.F , 0.F , 0.F);
	points[2] = XMFLOAT4(0.F ,0.F , 0.F , 0.F);
	points[3] = XMFLOAT4(0.F ,0.F , 0.F , 0.F);
	level = 0;
}

gjkSimplex::~gjkSimplex()
{
	
}

void gjkSimplex::addPoint(XMVECTOR point)
{
	XMStoreFloat4(&points[level], point);
	level++;
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point)
{
	level = 1;
	XMStoreFloat4(&points[0], point);
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1)
{
	level = 2;
	XMStoreFloat4(&points[0], point0);
	XMStoreFloat4(&points[1], point1);
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1, DirectX::XMVECTOR point2)
{
	level = 3;
	XMStoreFloat4(&points[0], point0);
	XMStoreFloat4(&points[1], point1);
	XMStoreFloat4(&points[2], point2);
}

void gjkSimplex::SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1, DirectX::XMVECTOR point2 , DirectX::XMVECTOR point3)
{
	level = 4;
	XMStoreFloat4( &points[0],point0);
	XMStoreFloat4( &points[1],point1);
	XMStoreFloat4( &points[2],point2);
	XMStoreFloat4( &points[3],point3);
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

	XMVECTOR startVector = convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A);
	simplex.addPoint(startVector);
	
	direction = XMVector3Normalize(-startVector);

	XMVECTOR farVector;
	int printCnt = 0;
	while (true)
	{	
		
		farVector = convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A);
		if (XMVector3Dot(farVector, direction).m128_f32[0] >= EPSILON)
		{
			return false;
		}
		

		for (int i = 0; i < simplex.level; i++)
		{
			XMVECTOR existingPoint = XMLoadFloat4(&simplex.points[i]);
			XMVECTOR lenSq = XMVector3LengthSq(existingPoint - farVector);

			if (lenSq.m128_f32[0]  <= EPSILON)
			{

				return true;
			}

		}

		if (simplex.level == 2)
		{
			XMVECTOR existingLine0 = XMLoadFloat4(&simplex.points[0]);
			XMVECTOR existingLine1 = XMLoadFloat4(&simplex.points[1]);

			if (XMVector3LinePointDistance(existingLine0, existingLine1, farVector).m128_f32[0] <= EPSILON)
			{
				XMVECTOR line = existingLine1 - existingLine0;
				XMVECTOR originDir = ORIGIN - existingLine0;
				
				if (XMVector3LengthSq(XMVector3Cross(line, originDir)).m128_f32[0] <= EPSILON * EPSILON)
				{
					return true;
				}
				else
				{
					return false;
				}
				
			}
		}
		else if (simplex.level == 3)
		{
			XMVECTOR existingPoint0 = XMLoadFloat4(&simplex.points[0]);
			XMVECTOR existingPoint1 = XMLoadFloat4(&simplex.points[1]);
			XMVECTOR existingPoint2 = XMLoadFloat4(&simplex.points[2]);
			XMVECTOR planeNormal = XMVector3Normalize(XMVector3Cross(existingPoint1 - existingPoint0 , existingPoint2 - existingPoint0));
			float distanceSq = XMVector3Dot(planeNormal, farVector - existingPoint0).m128_f32[0];
			distanceSq *= distanceSq;

			if (distanceSq <= EPSILON * EPSILON)
			{
				float dist = XMVector3Dot(planeNormal, ORIGIN - existingPoint0).m128_f32[0];
				if (dist * dist <= EPSILON * EPSILON)
				{
					return true;
				}
				else
				{
					return false;
				}
			
				
			}
		}
		
		//simplex 확장
		simplex.addPoint(convexB->Support(-direction, matTRS_B) - convexA->Support(direction, matTRS_A));

		if (selectSimplexHandle(simplex, direction))
		{// 0점을 찾아낸 경우
		
			return true;
		}
		

	}
	
	


}

bool HandleSimplexPoint(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	XMVECTOR point = XMLoadFloat4( &simplex.points[0]);

	if (XMVector3Equal(point, ORIGIN))
	{
		return true;
	}

	direction = XMVector3Normalize(ORIGIN - point);
	return false;
}

bool HandleSimplexLine(gjkSimplex& simplex, DirectX::XMVECTOR& direction)
{
	XMVECTOR pointA = XMLoadFloat4( &simplex.points[0]);
	XMVECTOR pointB = XMLoadFloat4( &simplex.points[1]);
	XMVECTOR lineBA = pointB - pointA;
	XMVECTOR lineAO = ORIGIN - pointA;

	direction = XMVector3Cross(XMVector3Cross(lineBA , lineAO) , lineBA);

	if (XMVector3LengthSq(direction).m128_f32[0] < EPSILON)
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
	XMVECTOR pointA = XMLoadFloat4( &simplex.points[0] );
	XMVECTOR pointB = XMLoadFloat4( &simplex.points[1] );
	XMVECTOR pointC = XMLoadFloat4( &simplex.points[2] );

	XMVECTOR lineAB = pointB - pointA;
	XMVECTOR lineAC = pointC - pointA;
	XMVECTOR lineAO = ORIGIN - pointA;

	XMVECTOR triNormal = XMVector3Cross(lineAB , lineAC);
	
	if (XMVector3Dot(triNormal, lineAO).m128_f32[0] > EPSILON)
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
	XMVECTOR point0 = XMLoadFloat4( &simplex.points[0] );
	XMVECTOR point1 = XMLoadFloat4( &simplex.points[1] );
	XMVECTOR point2 = XMLoadFloat4( &simplex.points[2] );
	XMVECTOR point3 = XMLoadFloat4( &simplex.points[3] );

	/*
	printf("%f %f %f | %f %f %f | %f %f %f | %f %f %f \n"
		,point0.m128_f32[0], point0.m128_f32[1], point0.m128_f32[2]
		, point1.m128_f32[0], point1.m128_f32[1], point1.m128_f32[2]
		, point2.m128_f32[0], point2.m128_f32[1], point2.m128_f32[2]
		, point3.m128_f32[0], point3.m128_f32[1], point3.m128_f32[2]);
	*/
	XMVECTOR face[4][3] = {
		{point0 ,point2 ,point1 },
		{point0 ,point3 ,point2 },
		{point0 ,point1 ,point3 },
		{point1 ,point2 ,point3 },
	};

	memcpy(simplex.faces , face , sizeof(XMVECTOR)*12);

	bool check = false;

	for (int i = 0; i < 4; i++)
	{
		XMVECTOR faceNorm = XMVector3Cross(face[i][1] - face[i][0], face[i][2] - face[i][0]);
	
		XMVECTOR p0 = face[i][0];
		if (XMVector3Dot(faceNorm, p0).m128_f32[0] < -EPSILON)
		{
			faceNorm = -faceNorm;
		}

		if (XMVector3Dot(faceNorm , ORIGIN - p0).m128_f32[0] > EPSILON)
		{
			direction = XMVector3Normalize(faceNorm);
			simplex.SetPoints(face[i][0], face[i][1], face[i][2]);

			check = true;
		}
		else // 퇴화된 삼각형 
		{
		}

	}


	if (check) return false;
	else return true;

}