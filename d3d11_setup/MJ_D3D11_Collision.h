#ifndef CAPSULE_COLLISION_H
#define CAPSULE_COLLISION_H
#include "VertexType.h"
#define SWEEP_HIT_FACE 0
#define SWEEP_HIT_EDGE 1
#define SWEEP_HIT_DOT 2

namespace CapsuleCollision
{

	typedef struct COLLISION_INFO {
		float penetrationDepth;
		DirectX::XMVECTOR normal;
		DirectX::XMVECTOR rayPoint;
		VERTEX_T tri[3];
	}COLLISION_INFO_T;

	typedef struct CAPSULE
	{
		DirectX::XMVECTOR head;
		DirectX::XMVECTOR foot;
		DirectX::XMVECTOR axis;// head - foot
	
		float radius;

	}CAPSULE_T;

	typedef struct SWEEP_HIT
	{
		float hitTime; // [0~1]으로 이동 구간에서 충돌 시점
		DirectX::XMVECTOR normal;
		DirectX::XMVECTOR point;
		DirectX::XMVECTOR tri[3];
		DirectX::XMVECTOR edge[2];
		float penetrationDepth;
		int element; // 점 선 면 충돌 구분
	}SWEEP_HIT_T;

	float TwoLineDistance(DirectX::XMVECTOR linePointA0, DirectX::XMVECTOR linePointA1, DirectX::XMVECTOR linePointB0, DirectX::XMVECTOR linePointB1, float& lineAParmeter, float& lineBParmeter);

	float LineTriDistance(DirectX::XMVECTOR linePoint1, DirectX::XMVECTOR linePoint2, DirectX::XMVECTOR* triPoint);
	
	class CapsuleCollider
	{
	public:
		CapsuleCollider();
		CapsuleCollider(CAPSULE_T capsule);
		~CapsuleCollider();
		
		CAPSULE Collider;
		DirectX::XMVECTOR nextMove;
		DirectX::XMVECTOR boxMax;
		DirectX::XMVECTOR boxMin;
		bool TriAngleCollisionTest(DirectX::XMVECTOR* triPos, CAPSULE_T& NextPos,SWEEP_HIT* outSwpInfo);
		bool TriAngleCollisionTest(DirectX::XMVECTOR* triPos);
		
		void SetCapsuleBox();
		void CollisionSlide(DirectX::XMVECTOR normal,DirectX::XMVECTOR moveVec);
		bool BoxTest(DirectX::XMVECTOR boxMax , DirectX::XMVECTOR boxMin);
	};

	

}


#endif // !CAPSULE_COLLISION_H
