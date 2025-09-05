#include "MJ_D3D11_Collision.h"
#include <string.h>
#include <stdio.h>
#include <EASTL/algorithm.h>
using namespace DirectX;
using namespace CapsuleCollision;


#define EPSILON 1e-6f

CapsuleCollider::CapsuleCollider()
{
	this->Collider.head = { 0.F , 1.F , 0.F , 1.F };
	this->Collider.foot = { 0.F ,-1.F , 0.F , 1.F };
	this->Collider.axis = { 0.F , 2.F , 0.F , 0.F };
	this->Collider.radius = 1.F;
	boxMax = { 
		this->Collider.radius,
		this->Collider.head.m128_f32[1] + this->Collider.radius,
		this->Collider.radius,
		0.F
	};

	boxMin = {
		-this->Collider.radius,
		this->Collider.foot.m128_f32[1] - this->Collider.radius,
		-this->Collider.radius,
		0.F
	};

	this->nextMove = {0 , 0 , 0, 0};
}

CapsuleCollider::~CapsuleCollider()
{

}
CapsuleCollider::CapsuleCollider(CAPSULE_T capsule)
{
	this->Collider = capsule;
	this->Collider.axis = this->Collider.head - this->Collider.foot;
	this->Collider.axis.m128_f32[3] = 0.F;

	boxMax = {
		this->Collider.head.m128_f32[0] + this->Collider.radius,
		this->Collider.head.m128_f32[1] + this->Collider.radius,
		this->Collider.head.m128_f32[2] + this->Collider.radius,
		0.F
	};

	boxMin = {
		this->Collider.foot.m128_f32[0] - this->Collider.radius,
		this->Collider.foot.m128_f32[1] - this->Collider.radius,
		this->Collider.foot.m128_f32[2] - this->Collider.radius,
		0.F
	};
}


void CapsuleCollider::SetCapsuleBox()
{
	boxMax = {
		this->Collider.head.m128_f32[0] + this->Collider.radius,
		this->Collider.head.m128_f32[1] + this->Collider.radius,
		this->Collider.head.m128_f32[2] + this->Collider.radius,
		0.F
	};

	boxMin = {
		this->Collider.foot.m128_f32[0] - this->Collider.radius,
		this->Collider.foot.m128_f32[1] - this->Collider.radius,
		this->Collider.foot.m128_f32[2] - this->Collider.radius,
		0.F
	};
}



bool IsPointInTriAngle(XMVECTOR TriVector[3] , XMVECTOR Point)
{
	XMVECTOR pointLineT0 = Point - TriVector[0];
	XMVECTOR pointLineT1 = Point - TriVector[1];
	XMVECTOR pointLineT2 = Point - TriVector[2];

	XMVECTOR edge0 = TriVector[1] - TriVector[0];
	XMVECTOR edge1 = TriVector[2] - TriVector[1];
	XMVECTOR edge2 = TriVector[0] - TriVector[2];

	XMVECTOR triNormal = XMVector3Normalize( XMVector3Cross(edge0 , edge1) );

	XMVECTOR dir0 = XMVector3Dot(triNormal , XMVector3Cross(edge0, pointLineT0));
	XMVECTOR dir1 = XMVector3Dot(triNormal, XMVector3Cross(edge1, pointLineT1));
	XMVECTOR dir2 = XMVector3Dot(triNormal, XMVector3Cross(edge2, pointLineT2));


	if ((dir0.m128_f32[0] >= -EPSILON) &&
		(dir1.m128_f32[0] >= -EPSILON) &&
		(dir2.m128_f32[0] >= -EPSILON))
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool IsSphereTriAngleSweepHitFace(XMVECTOR curPoint, XMVECTOR nextPoint, float radius, XMVECTOR* triPos, SWEEP_HIT_T* swpInfo)
{
	XMVECTOR normal = XMVector3Cross((triPos[1] - triPos[0]), (triPos[2] - triPos[0]));
	XMVECTOR move = nextPoint - curPoint;

	if (XMVector3Length(normal).m128_f32[0] < EPSILON)
	{
		return false;
	}
	normal = XMVector3Normalize(normal);

	float denom = XMVectorGetX(XMVector3Dot(move, normal));
	if ( (denom * denom) < EPSILON || denom >= 0) return false;
	
	float rayGradient = (radius - XMVector3Dot((curPoint - triPos[0]), normal).m128_f32[0]) / denom;
	if (rayGradient < 0.F || rayGradient > 1.F) return false;


	XMVECTOR rayPoint = (curPoint - normal * radius) + (move)*rayGradient;
	if (!IsPointInTriAngle(triPos, rayPoint))
		return false;
	
	swpInfo->element = SWEEP_HIT_FACE;
	swpInfo->normal = normal;
	swpInfo->point = rayPoint;
	swpInfo->hitTime = rayGradient;
	return true;
}


bool IsSphereTriAngleSweepHitEdge(XMVECTOR curPoint, XMVECTOR nextPoint, float radius, XMVECTOR* triPos)
{
	for (int i = 0; i < 3; i++)
	{
		float dist = TwoLineDistance(curPoint, nextPoint, triPos[i % 3], triPos[(i + 1) % 3]);
		if (dist < radius)
			return true;
	}
	return false;
}

bool IsSphereTriAngleSweepHitVertex(XMVECTOR curPoint, XMVECTOR nextPoint, float radius, XMVECTOR* triPos)
{
	for (int i = 0; i < 3; i++)
	{
		float dist = XMVector3LinePointDistance(curPoint, nextPoint, triPos[i]).m128_f32[0];
		if (dist < radius)
			return true;
	}
	return false;
}


bool CapsuleCollider::TriAngleCollisionTest(VERTEX_T* TriVertexs, CAPSULE_T& NextPos , XMVECTOR* RayPoint, XMVECTOR* Normal)
{
	bool footTest = true;
	bool headTest = true;
	if (XMVector3Length(this->Collider.foot - NextPos.foot).m128_f32[0] < EPSILON)
		return false;

	XMVECTOR triPos[3] = {	{TriVertexs[0].pos.x , TriVertexs[0].pos.y , TriVertexs[0].pos.z , 1.F} , 
							{TriVertexs[1].pos.x , TriVertexs[1].pos.y , TriVertexs[1].pos.z , 1.F} ,
							{TriVertexs[2].pos.x , TriVertexs[2].pos.y , TriVertexs[2].pos.z , 1.F} };

	XMVECTOR triFaceNormal = XMVector3Cross((triPos[1] - triPos[0]) , (triPos[2] - triPos[0]));
	if (XMVector3Length(triFaceNormal).m128_f32[0] < EPSILON) return false;

	triFaceNormal = XMVector3Normalize(triFaceNormal)*this->Collider.radius;
	//foot sphere
	XMVECTOR denom = XMVector3Dot((NextPos.foot - this->Collider.foot), triFaceNormal);
	float footRayGradient = 0.F;
	if (XMVector3Length(denom).m128_f32[0] < EPSILON)
	{
		footTest = false;
		
	}
	else
	{
		footRayGradient = (-1.F * XMVector3Dot((this->Collider.foot - triFaceNormal - triPos[0]), triFaceNormal) / denom).m128_f32[0];
		if (footRayGradient < 0 || footRayGradient > 1.F)footTest = false;
	
	}
	//printf("%f \n", denom.m128_f32[0]);
	XMVECTOR footRayPoint = (this->Collider.foot - triFaceNormal) + (NextPos.foot - this->Collider.foot) * footRayGradient;

	*RayPoint = footRayPoint;
	*Normal = triFaceNormal;
	
	
	if (footTest &&
		((XMVector3Dot(NextPos.foot - this->Collider.foot, footRayPoint - this->Collider.foot).m128_f32[0] > 0) &&
			(XMVector3Length(NextPos.foot - this->Collider.foot).m128_f32[0] > XMVector3Length(footRayPoint - (this->Collider.foot - triFaceNormal)).m128_f32[0]) &&
			IsPointInTriAngle(triPos, footRayPoint))
		)
	{
		//printf("foot Ãæµ¹\n");
		return true;
	}
	//foot-head silinder 
	XMVECTOR axisLenNorm = XMVector3Dot(this->Collider.head - this->Collider.foot, this->Collider.head - this->Collider.foot);
	XMVECTOR axisNormal = this->Collider.head - this->Collider.foot;

	XMVECTOR curDotV0 = XMVector3Dot(triPos[0] - this->Collider.foot, this->Collider.head - this->Collider.foot);
	XMVECTOR curPointV0 = axisNormal * (curDotV0.m128_f32[0] / axisLenNorm.m128_f32[0]) + this->Collider.foot;

	XMVECTOR nextDotV0 = XMVector3Dot(triPos[0] - NextPos.foot, NextPos.head - NextPos.foot);
	XMVECTOR nextPointV0 = axisNormal * (nextDotV0.m128_f32[0] / axisLenNorm.m128_f32[0]) + NextPos.foot;

	XMVECTOR curDotV1 = XMVector3Dot(triPos[1] - this->Collider.foot, this->Collider.head - this->Collider.foot);
	XMVECTOR curPointV1 = axisNormal * (curDotV1.m128_f32[0] / axisLenNorm.m128_f32[0]) + this->Collider.foot;

	XMVECTOR nextDotV1 = XMVector3Dot(triPos[1] - NextPos.foot, NextPos.head - NextPos.foot);
	XMVECTOR nextPointV1 = axisNormal * (nextDotV1.m128_f32[0] / axisLenNorm.m128_f32[0]) + NextPos.foot;

	XMVECTOR curDotV2 = XMVector3Dot(triPos[2] - this->Collider.foot, this->Collider.head - this->Collider.foot);
	XMVECTOR curPointV2 = axisNormal * (curDotV2.m128_f32[0] / axisLenNorm.m128_f32[0]) + this->Collider.foot;

	XMVECTOR nextDotV2 = XMVector3Dot(triPos[2] - NextPos.foot, NextPos.head - NextPos.foot);
	XMVECTOR nextPointV2 = axisNormal * (nextDotV2.m128_f32[0] / axisLenNorm.m128_f32[0]) + NextPos.foot;

	bool silinderTest = (
		((XMVector3Dot(nextPointV0 - curPointV0, triPos[0] - curPointV0).m128_f32[0] > 0) && ((XMVector3Length(triPos[0] - curPointV0).m128_f32[0] < this->Collider.radius) && (curDotV0.m128_f32[0] > 0) && (curDotV0.m128_f32[0] < axisLenNorm.m128_f32[0]))) ||
		((XMVector3Dot(nextPointV1 - curPointV1, triPos[1] - curPointV1).m128_f32[0] > 0) && ((XMVector3Length(triPos[1] - curPointV1).m128_f32[0] < this->Collider.radius) && (curDotV1.m128_f32[0] > 0) && (curDotV1.m128_f32[0] < axisLenNorm.m128_f32[0]))) ||
		((XMVector3Dot(nextPointV2 - curPointV2, triPos[2] - curPointV2).m128_f32[0] > 0) && ((XMVector3Length(triPos[2] - curPointV2).m128_f32[0] < this->Collider.radius) && (curDotV2.m128_f32[0] > 0) && (curDotV2.m128_f32[0] < axisLenNorm.m128_f32[0])))
		);
	if (silinderTest) {		
		//printf("silinder Ãæµ¹\n");
		return true; 
	}

	//head sphere

	denom = XMVector3Dot((NextPos.head - this->Collider.head), triFaceNormal);
	XMVECTOR headRayGradient = -1.F * XMVector3Dot((this->Collider.head - triFaceNormal - triPos[0]), triFaceNormal) / denom;
	
	XMVECTOR headRayPoint = (this->Collider.head - triFaceNormal) + (NextPos.head - this->Collider.head) * headRayGradient;
	*RayPoint = headRayPoint;
	if (
		(XMVector3Dot(NextPos.head - this->Collider.head, headRayPoint - this->Collider.head).m128_f32[0] > 0) &&
		(XMVector3Length(NextPos.head - this->Collider.head).m128_f32[0] > XMVector3Length(headRayPoint - (this->Collider.head - triFaceNormal)).m128_f32[0]) &&
		IsPointInTriAngle(triPos, headRayPoint)
		) 
	{
		//printf("head Ãæµ¹\n");
		return true; 
	}
	return false;
}

float CapsuleCollision::TwoLineDistance(XMVECTOR linePointA0, XMVECTOR linePointA1, XMVECTOR linePointB0, XMVECTOR linePointB1)
{
	XMVECTOR lineA = (linePointA1 - linePointA0);
	XMVECTOR lineB = (linePointB1 - linePointB0);
	XMVECTOR lineA0B0 = linePointB0-linePointA0;

	float aaDt = XMVector3Dot(lineA, lineA).m128_f32[0];
	float abDt = XMVector3Dot(lineA, lineB).m128_f32[0];
	float bbDt = XMVector3Dot(lineB, lineB).m128_f32[0];
	float a0Dt = XMVector3Dot(lineA, lineA0B0).m128_f32[0];
	float b0Dt = XMVector3Dot(lineB, lineA0B0).m128_f32[0];


	if (aaDt <= EPSILON && bbDt <= EPSILON)
	{
		return XMVector3Length(linePointA0 - linePointB0).m128_f32[0];
	}
	
	if (fabsf(aaDt * bbDt - abDt * abDt) > EPSILON)
	{
		float lineAParmeter = (abDt * b0Dt - bbDt * a0Dt) / (aaDt * bbDt - abDt * abDt);
		float lineBParmeter = (aaDt * b0Dt - abDt * a0Dt) / (aaDt * bbDt - abDt * abDt);
		
		if (lineAParmeter < 0) lineAParmeter = 0;
		else if (lineAParmeter > 1.F) lineAParmeter = 1.F;

		if (lineBParmeter < 0) lineBParmeter = 0;
		else if (lineBParmeter > 1.F) lineBParmeter = 1.F;

		XMVECTOR pointA = linePointA0 + lineAParmeter * lineA;
		XMVECTOR pointB = linePointB0 + lineBParmeter * lineB;
		
		return XMVector3Length(pointA - pointB).m128_f32[0];
	}
	else // ÆòÇà ÇÑ °æ¿ì
	{
		float d1 = XMVector3LinePointDistance(linePointA0, linePointA1, linePointB0).m128_f32[0];
		float d2 = XMVector3LinePointDistance(linePointA0, linePointA1, linePointB1).m128_f32[0];
		float d3 = XMVector3LinePointDistance(linePointB0, linePointB1, linePointA0).m128_f32[0];
		float d4 = XMVector3LinePointDistance(linePointB0, linePointB1, linePointA1).m128_f32[0];
		return eastl::min(eastl::min(d1, d2), eastl::min(d3 , d4));


	}
	

}
float CapsuleCollision::LineTriDistance(XMVECTOR linePoint1 ,XMVECTOR linePoint2,XMVECTOR* triPoint)
{

	XMVECTOR line = linePoint2 - linePoint1;
	float minDistance = 9999;
	
	for (int i = 0; i < 3; i++)
	{
		float distance = TwoLineDistance(linePoint1, linePoint2 , triPoint[(i+1)%3] , triPoint[i % 3]);
		if (distance < minDistance) minDistance = distance;
	}
	
	return minDistance;
}

bool CapsuleCollider::TriAngleCollisionTest(VERTEX_T* TriVertexs)
{
	
	XMVECTOR triPos[3] = { {TriVertexs[0].pos.x , TriVertexs[0].pos.y , TriVertexs[0].pos.z , 1.F} ,
							{TriVertexs[1].pos.x , TriVertexs[1].pos.y , TriVertexs[1].pos.z , 1.F} ,
						{TriVertexs[2].pos.x , TriVertexs[2].pos.y , TriVertexs[2].pos.z , 1.F} };
	
	//tri vertex-head foot distance
	for (int i = 0; i < 3; i++)
	{
		float footVertDst = XMVector3Length(this->Collider.foot - triPos[i]).m128_f32[0];
		float headVertDst = XMVector3Length(this->Collider.head - triPos[i]).m128_f32[0];
		if (footVertDst < this->Collider.radius || headVertDst < this->Collider.radius)
			return true;
	}

	XMVECTOR triFaceNormal = XMVector3Cross((triPos[1] - triPos[0]), (triPos[2] - triPos[0]));
	if (XMVector3Length(triFaceNormal).m128_f32[0] < EPSILON) return false;

	triFaceNormal = XMVector3Normalize(triFaceNormal);


	//foot sphere Triangle
	float footTriDistance = XMVector3Dot(triPos[0] - this->Collider.foot ,-triFaceNormal).m128_f32[0];
	if (footTriDistance < 0) footTriDistance = -footTriDistance;
	
	XMVECTOR footTriPoint = -triFaceNormal * footTriDistance + this->Collider.foot;
	
	if (IsPointInTriAngle(triPos, footTriPoint) && footTriDistance < this->Collider.radius - EPSILON)
	{
		printf("!foot °ãÄ§ %f  %f! TRI:(%f %f %f| %f %f %f| %f %f %f)  \n", footTriDistance, this->Collider.radius - EPSILON,
			TriVertexs[0].pos.x, TriVertexs[0].pos.y, TriVertexs[0].pos.z,
			TriVertexs[1].pos.x, TriVertexs[1].pos.y, TriVertexs[1].pos.z,
			TriVertexs[2].pos.x, TriVertexs[2].pos.y, TriVertexs[2].pos.z
		);

		return true;
	}
		
	//head sphere Triangle
	float headTriDistance = XMVector3Dot(triPos[0] - this->Collider.head, -triFaceNormal).m128_f32[0];
	if (headTriDistance < 0) headTriDistance = -headTriDistance;

	XMVECTOR headTriPoint = -triFaceNormal * headTriDistance + this->Collider.head;

	if (IsPointInTriAngle(triPos, headTriPoint) && headTriDistance < this->Collider.radius - EPSILON)
	{
		printf("!head °ãÄ§ ! \n");
		return true;
	}


	//sillinder Triangle
	float sillinderTriDistance = LineTriDistance(this->Collider.foot , this->Collider.head , triPos);
	
	if (sillinderTriDistance < this->Collider.radius - EPSILON)
	{
		//printf("!sillinder °Å¸® °ãÄ§ %f! \n", sillinderTriDistance);
		return true;
	}




	XMVECTOR sillinderRayGradient = -1.F * XMVector3Dot((this->Collider.head - triFaceNormal - triPos[0]), triFaceNormal) / XMVector3Dot((this->Collider.foot - this->Collider.head), triFaceNormal);
	XMVECTOR sillinderRayPoint = (this->Collider.head - triFaceNormal) + (this->Collider.foot - this->Collider.head) * sillinderRayGradient;
	if (IsPointInTriAngle(triPos, sillinderRayPoint) 
		&& sillinderRayPoint.m128_f32[1] > (this->Collider.foot.m128_f32[1])
		&& sillinderRayPoint.m128_f32[1] < (this->Collider.head.m128_f32[1])
		)
	{
		/*
		printf("!sillinder »ï°¢Çü ±³Â÷ °ãÄ§(%f %f %f) (%f %f %f) ! \n",
			this->Collider.foot.m128_f32[0],
			this->Collider.foot.m128_f32[1],
			this->Collider.foot.m128_f32[2],
			sillinderRayPoint.m128_f32[0],
			sillinderRayPoint.m128_f32[1],
			sillinderRayPoint.m128_f32[2]);
		*/
		return true;
	}

	return false;
}



void CapsuleCollider::CollisionSlide(VERTEX_T* TriVertexs, CAPSULE_T& NextPos, XMVECTOR rayPoint )
{
	XMVECTOR triPos[3] = {  {TriVertexs[0].pos.x , TriVertexs[0].pos.y , TriVertexs[0].pos.z , 1.F} ,
							{TriVertexs[1].pos.x , TriVertexs[1].pos.y , TriVertexs[1].pos.z , 1.F} ,
							{TriVertexs[2].pos.x , TriVertexs[2].pos.y , TriVertexs[2].pos.z , 1.F} };

	XMVECTOR triFaceNormal = XMVector3Cross((triPos[1] - triPos[0]), (triPos[2] - triPos[0]));
	triFaceNormal = XMVector3Normalize(triFaceNormal);

	XMVECTOR nextMove = NextPos.foot - Collider.foot;
	
	XMVECTOR finalMove = nextMove - (XMVector3Dot(triFaceNormal , nextMove) * triFaceNormal);
	finalMove.m128_f32[1] += EPSILON;
	this->nextMove = finalMove;
}


bool CapsuleCollider::BoxTest(DirectX::XMVECTOR boxMax, DirectX::XMVECTOR boxMin)
{
	this->SetCapsuleBox();
	
	bool result = (this->boxMin.m128_f32[0] <= boxMax.m128_f32[0] && this->boxMax.m128_f32[0] >= boxMin.m128_f32[0]) &&
		(this->boxMin.m128_f32[1] <= boxMax.m128_f32[1] && this->boxMax.m128_f32[1] >= boxMin.m128_f32[1]) &&
		(this->boxMin.m128_f32[2] <= boxMax.m128_f32[2] && this->boxMax.m128_f32[2] >= boxMin.m128_f32[2]);



	return result;
}


