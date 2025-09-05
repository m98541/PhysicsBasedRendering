#ifndef MAPPARTITIONING_H
#define MAPPARTITIONING_H
#include "VertexType.h"
#include "MJ_D3D11_BVTree.h"
#include "MJ_D3D11_Collision.h"

namespace Map {
	class MapObject
	{
	public:
		MapObject(DirectX::XMFLOAT4* inMapData, int size);
		MapObject();
		~MapObject();

		TRIPOLY_T* mMapDataBuffer;
		int mMapDataBufferCount;
		BVtree::BvNode* mBoxVolumeTree;

		void SetMapData(DirectX::XMFLOAT4* inMapData, int size);
		bool IsMapSwpCollisionDetect(CapsuleCollision::CapsuleCollider& player, CapsuleCollision::CAPSULE nextCapsule , eastl::vector<CapsuleCollision::SWEEP_HIT_T>& outSwpSet);
		void MapBuild(void);

	};
	float ComputePenetration(CapsuleCollision::SWEEP_HIT_T& swpHit, CapsuleCollision::CAPSULE_T& capsule);
}



#endif // !MAPPARTITIONING_H
