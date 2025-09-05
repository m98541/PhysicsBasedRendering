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
	curPoint.m128_f32[3] = 0.F;
	nextPoint.m128_f32[3] = 0.F;

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
	swpInfo->tri[0] = triPos[0];
	swpInfo->tri[1] = triPos[1];
	swpInfo->tri[2] = triPos[2];
	swpInfo->normal = normal;
	swpInfo->point = rayPoint;
	swpInfo->element = SWEEP_HIT_FACE;
	
	swpInfo->hitTime = rayGradient;

	
	return true;
}


bool IsSphereTriAngleSweepHitEdge(XMVECTOR curPoint, XMVECTOR nextPoint, float radius, XMVECTOR* triPos, SWEEP_HIT_T* swpInfo)
{
	curPoint.m128_f32[3] = 0.F;
	nextPoint.m128_f32[3] = 0.F;

	float lineP0, lineP1;

	for (int i = 0; i < 3; i++)
	{
		float dist = TwoLineDistance(curPoint, nextPoint, triPos[i % 3], triPos[(i + 1) % 3], lineP0 , lineP1);
		
		if (dist <= radius)
		{
			//printf("선충돌\n");

			swpInfo->tri[0] = triPos[0]; 
			swpInfo->tri[1] = triPos[1];
			swpInfo->tri[2] = triPos[2];
			swpInfo->element = SWEEP_HIT_EDGE;
			XMVECTOR line = triPos[(i + 1) % 3] - triPos[i % 3];
			XMVECTOR point = line * lineP1 + triPos[i % 3];
			swpInfo->point = point;
			point.m128_f32[3] = 0;
			XMVECTOR n = curPoint - point;
			n.m128_f32[3] = 0;
			swpInfo->normal = XMVector3Normalize(n);
			
			swpInfo->hitTime = lineP0;
			if (swpInfo->hitTime < 0 || swpInfo->hitTime > 1) continue;
			return true;
		}
			
	}
	return false;
}

bool IsSphereTriAngleSweepHitVertex(XMVECTOR curPoint, XMVECTOR nextPoint, float radius, XMVECTOR* triPos, SWEEP_HIT_T* swpInfo)
{
	curPoint.m128_f32[3] = 0.F;
	nextPoint.m128_f32[3] = 0.F;

	XMVECTOR line = nextPoint - curPoint;

	for (int i = 0; i < 3; i++)
	{

		XMVECTOR lineP1 = triPos[i % 3] - curPoint;
		
		float dist = XMVector3LinePointDistance(curPoint, nextPoint, triPos[i]).m128_f32[0];
		if (dist <= radius)
		{
			//printf("점충돌\n");
			
			swpInfo->tri[0] = triPos[0];
			swpInfo->tri[1] = triPos[1];
			swpInfo->tri[2] = triPos[2];
			swpInfo->element = SWEEP_HIT_DOT;
		
			float lineLen = XMVector3Dot(line, line).m128_f32[0];
			if (lineLen == 0) 
				swpInfo->hitTime = 0;
			else
				swpInfo->hitTime = XMVector3Dot(triPos[i] - curPoint, nextPoint - curPoint).m128_f32[0] / lineLen;

			if (swpInfo->hitTime < 0 || swpInfo->hitTime > 1) continue;
			else
			{
				swpInfo->point = line * swpInfo->hitTime + curPoint;
				XMVECTOR n = (swpInfo->point- triPos[i]);
				n.m128_f32[3] = 0;
				swpInfo->normal = XMVector3Normalize(n);
			}
			return true;
		}
			
	}
	return false;
}


bool CapsuleCollider::TriAngleCollisionTest(XMVECTOR* triPos, CAPSULE_T& NextPos ,SWEEP_HIT* outSwpInfo)
{
	bool footTest = true;
	bool headTest = true;
	if (XMVector3Length(this->Collider.foot - NextPos.foot).m128_f32[0] < EPSILON)
		return true;

	
	SWEEP_HIT swpInfo;


	if (
		IsSphereTriAngleSweepHitFace(this->Collider.foot, NextPos.foot, this->Collider.radius, triPos, &swpInfo) ||
		IsSphereTriAngleSweepHitEdge(this->Collider.foot, NextPos.foot, this->Collider.radius, triPos, &swpInfo) ||
		IsSphereTriAngleSweepHitVertex(this->Collider.foot, NextPos.foot, this->Collider.radius, triPos, &swpInfo)
		)
	{

		*outSwpInfo = swpInfo;

		return true;
	}

	if (
		IsSphereTriAngleSweepHitFace(this->Collider.head, NextPos.head, this->Collider.radius, triPos, &swpInfo) ||
		IsSphereTriAngleSweepHitEdge(this->Collider.head, NextPos.head, this->Collider.radius, triPos , &swpInfo) ||
		IsSphereTriAngleSweepHitVertex(this->Collider.head, NextPos.head, this->Collider.radius, triPos , &swpInfo)
		)
	{
		*outSwpInfo = swpInfo;
		return true;
	}

	//foot-head cyilinder 
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

	

	bool cyilinderTest = (
		((XMVector3Dot(nextPointV0 - curPointV0, triPos[0] - curPointV0).m128_f32[0] > 0) && ((XMVector3Length(triPos[0] - curPointV0).m128_f32[0] < this->Collider.radius) && (curDotV0.m128_f32[0] > 0) && (curDotV0.m128_f32[0] < axisLenNorm.m128_f32[0]))) ||
		((XMVector3Dot(nextPointV1 - curPointV1, triPos[1] - curPointV1).m128_f32[0] > 0) && ((XMVector3Length(triPos[1] - curPointV1).m128_f32[0] < this->Collider.radius) && (curDotV1.m128_f32[0] > 0) && (curDotV1.m128_f32[0] < axisLenNorm.m128_f32[0]))) ||
		((XMVector3Dot(nextPointV2 - curPointV2, triPos[2] - curPointV2).m128_f32[0] > 0) && ((XMVector3Length(triPos[2] - curPointV2).m128_f32[0] < this->Collider.radius) && (curDotV2.m128_f32[0] > 0) && (curDotV2.m128_f32[0] < axisLenNorm.m128_f32[0])))
		);
	if (cyilinderTest) {		
		printf("cyilinder 충돌\n");
		swpInfo.element = SWEEP_HIT_FACE;
		swpInfo.tri[0] = triPos[0];
		swpInfo.tri[1] = triPos[1];
		swpInfo.tri[2] = triPos[2];
		return true; 
	}

	//head sphere
	return false;
}

float CapsuleCollision::TwoLineDistance(XMVECTOR linePointA0, XMVECTOR linePointA1, XMVECTOR linePointB0, XMVECTOR linePointB1,float& lineAParmeter, float& lineBParmeter)
{
	XMVECTOR lineA = (linePointA1 - linePointA0);
	XMVECTOR lineB = (linePointB1 - linePointB0);
	XMVECTOR lineA0B0 = linePointA0 - linePointB0;

	float aaDt = XMVector3Dot(lineA, lineA).m128_f32[0];
	float abDt = XMVector3Dot(lineA, lineB).m128_f32[0];
	float bbDt = XMVector3Dot(lineB, lineB).m128_f32[0];
	float a0Dt = XMVector3Dot(lineA, lineA0B0).m128_f32[0];
	float b0Dt = XMVector3Dot(lineB, lineA0B0).m128_f32[0];

	if (aaDt <= EPSILON && bbDt <= EPSILON)
	{
		return XMVector3Length(linePointA0 - linePointB0).m128_f32[0];
	}
	
	if (aaDt <= EPSILON) // A 선분이 아주 작은 경우(사실상 점)
	{
		lineAParmeter = 0.F;
		lineBParmeter = eastl::clamp(b0Dt / bbDt , 0.F , 1.F);

	}
	else
	{
		if (bbDt <= EPSILON)
		{
			lineAParmeter = eastl::clamp(-a0Dt / aaDt, 0.F, 1.F);
			lineBParmeter = 0.F;
		}
		else
		{
			if (fabsf(aaDt * bbDt - abDt * abDt) > EPSILON)
			{
				lineAParmeter =eastl::clamp( (abDt * b0Dt - bbDt * a0Dt) / (aaDt * bbDt - abDt * abDt) , 0.F, 1.F);
				
			}
			else // 평행 한 경우
			{
				lineAParmeter = eastl::clamp(-a0Dt / aaDt , 0.F , 1.F);
			}


			if ((abDt * lineAParmeter + b0Dt) < 0.F)
			{
				lineBParmeter = 0;
				lineAParmeter = eastl::clamp(-a0Dt / aaDt , 0.F , 1.F);

			}
			else if ((abDt * lineAParmeter + b0Dt) > bbDt)
			{
				lineBParmeter = 1.F;
				lineAParmeter = eastl::clamp((abDt - a0Dt)/(aaDt), 0.F, 1.F);

			}
			else
			{
				lineBParmeter = (abDt * lineAParmeter + b0Dt) / bbDt;
			}




		}


	}


	XMVECTOR pointA = linePointA0 + lineAParmeter * lineA;
	XMVECTOR pointB = linePointB0 + lineBParmeter * lineB;
	return XMVector3Length(pointB - pointA).m128_f32[0];
	

}
float CapsuleCollision::LineTriDistance(XMVECTOR linePoint1 ,XMVECTOR linePoint2,XMVECTOR* triPoint)
{

	XMVECTOR line = linePoint2 - linePoint1;
	float minDistance = 9999;

	float lineP0, lineP1;

	for (int i = 0; i < 3; i++)
	{
		float distance = TwoLineDistance(linePoint1, linePoint2 , triPoint[(i+1)%3] , triPoint[i % 3], lineP0, lineP1);
		if (distance < minDistance) minDistance = distance;
	}
	
	return minDistance;
}

bool CapsuleCollider::TriAngleCollisionTest(XMVECTOR* triPos)
{
	

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
		return true;
	}
		
	//head sphere Triangle
	float headTriDistance = XMVector3Dot(triPos[0] - this->Collider.head, -triFaceNormal).m128_f32[0];
	if (headTriDistance < 0) headTriDistance = -headTriDistance;

	XMVECTOR headTriPoint = -triFaceNormal * headTriDistance + this->Collider.head;

	if (IsPointInTriAngle(triPos, headTriPoint) && headTriDistance < this->Collider.radius - EPSILON)
	{
		//printf("!head 겹침 ! \n");
		return true;
	}


	//sillinder Triangle
	float sillinderTriDistance = LineTriDistance(this->Collider.foot , this->Collider.head , triPos);
	
	if (sillinderTriDistance < this->Collider.radius - EPSILON)
	{
		//printf("!sillinder 거리 겹침 %f! \n", sillinderTriDistance);
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
		printf("!sillinder 삼각형 교차 겹침(%f %f %f) (%f %f %f) ! \n",
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



void CapsuleCollider::CollisionSlide(XMVECTOR normal,XMVECTOR moveVec )
{
	XMVECTOR finalMove = moveVec - (XMVector3Dot(normal, moveVec) * normal);
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


