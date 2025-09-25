#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include "MJ_D3D11_WorldMap.h"

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2
#define MIN_AXIS_SIZE 10

using namespace Map;
using namespace CapsuleCollision;
using namespace DirectX;

using namespace BVtree; // Corrected namespace usage
MapObject::MapObject()
{
	mMapDataBuffer = nullptr;
	mMapDataBufferCount = 0;
}

MapObject::MapObject(XMFLOAT4* inMapData, int size)
{
	this->SetMapData(inMapData , size);
}

MapObject::~MapObject()
{
	free(mMapDataBuffer);
}


void MapObject::SetMapData(XMFLOAT4* inMapData, int size)
{
	//root 1 부터 
	mMapDataBuffer = (TRIPOLY_T*)malloc(sizeof(TRIPOLY_T) * ((int)(size / 3) + 1));
	mMapDataBufferCount = size / 3;
	mBoxVolumeTree = new BvNode();

	mBoxVolumeTree->mMaxCoord = mBoxVolumeTree->mMinCoord = XMLoadFloat4(inMapData);
	
	mBoxVolumeTree->mDataStart = 0;
	mBoxVolumeTree->mDataEnd = mMapDataBufferCount;

	for (int i = 0; mMapDataBuffer != NULL && i < mMapDataBufferCount; i++)
	{
		mMapDataBuffer[i].Vertex[0] = inMapData[i*3 + 0];
		mMapDataBuffer[i].Centroid = XMLoadFloat4(inMapData + i*3 + 0);
		mMapDataBuffer[i].MaxCoord = mMapDataBuffer[i].MinCoord = XMLoadFloat4(inMapData + i * 3 + 0);

		mMapDataBuffer[i].Vertex[1] = inMapData[i*3 + 1];
		mMapDataBuffer[i].Centroid += XMLoadFloat4(inMapData + i*3 + 1);

		for (int j = 0; j < 3; j++)
		{
			mMapDataBuffer[i].MaxCoord.m128_f32[j] = (mMapDataBuffer[i].MaxCoord.m128_f32[j] < XMLoadFloat4(inMapData + i * 3 + 1).m128_f32[j]) ?
				XMLoadFloat4(inMapData + i * 3 + 1).m128_f32[j] : mMapDataBuffer[i].MaxCoord.m128_f32[j];

			mMapDataBuffer[i].MinCoord.m128_f32[j] = (mMapDataBuffer[i].MinCoord.m128_f32[j] > XMLoadFloat4(inMapData + i * 3 + 1).m128_f32[j]) ?
				XMLoadFloat4(inMapData + i * 3 + 1).m128_f32[j] : mMapDataBuffer[i].MinCoord.m128_f32[j];
		}	

		mMapDataBuffer[i].Vertex[2] = inMapData[i*3 + 2];
		mMapDataBuffer[i].Centroid += XMLoadFloat4(inMapData + i*3 + 2);

		for (int j = 0; j < 3; j++)
		{
			mMapDataBuffer[i].MaxCoord.m128_f32[j] = (mMapDataBuffer[i].MaxCoord.m128_f32[j] < XMLoadFloat4(inMapData + i * 3 + 2).m128_f32[j]) ?
				XMLoadFloat4(inMapData + i * 3 + 2).m128_f32[j] : mMapDataBuffer[i].MaxCoord.m128_f32[j];

			mMapDataBuffer[i].MinCoord.m128_f32[j] = (mMapDataBuffer[i].MinCoord.m128_f32[j] > XMLoadFloat4(inMapData + i * 3 + 2).m128_f32[j]) ?
				XMLoadFloat4(inMapData + i * 3 + 2).m128_f32[j] : mMapDataBuffer[i].MinCoord.m128_f32[j];
		}

		mMapDataBuffer[i].Centroid /= 3.0;
		mMapDataBuffer[i].Centroid.m128_f32[3] = 0.F;

		for (int j = 0; j < 3; j++)
		{
			if (mMapDataBuffer[i].MaxCoord.m128_f32[j] - mMapDataBuffer[i].MinCoord.m128_f32[j] < MIN_AXIS_SIZE * 2)
			{
				mMapDataBuffer[i].MaxCoord.m128_f32[j] += MIN_AXIS_SIZE;
				mMapDataBuffer[i].MinCoord.m128_f32[j] -= MIN_AXIS_SIZE;
			}
		}



		for (int j = 0; j < 3; j++)
		{
			mBoxVolumeTree->mMaxCoord.m128_f32[j] = (mBoxVolumeTree->mMaxCoord.m128_f32[j] < mMapDataBuffer[i].MaxCoord.m128_f32[j]) ?
				mMapDataBuffer[i].MaxCoord.m128_f32[j] : mBoxVolumeTree->mMaxCoord.m128_f32[j];

			mBoxVolumeTree->mMinCoord.m128_f32[j] = (mBoxVolumeTree->mMinCoord.m128_f32[j] > mMapDataBuffer[i].MinCoord.m128_f32[j]) ?
				mMapDataBuffer[i].MinCoord.m128_f32[j] : mBoxVolumeTree->mMinCoord.m128_f32[j];
		}

	}


}


void Partition(TRIPOLY_T* dataBuffer,int start,int end,int axis)
{
	if (end - start <= 1)
	{
		return;
	}

	eastl::sort(dataBuffer + start, dataBuffer + end, [axis](const TRIPOLY_T& lTri, const TRIPOLY_T& rTri) {
		return lTri.Centroid.m128_f32[axis] < rTri.Centroid.m128_f32[axis];
	});
	
	int mid = (start + end) / 2;

	
	Partition(dataBuffer, start, mid, (axis + 1) % 3);
	Partition(dataBuffer, mid, end , (axis + 1) % 3);
}

void BuildBVTree(TRIPOLY_T* dataBuffer,BvNode* tree,int start, int end)
{
	tree->mDataStart = start;
	tree->mDataEnd = end;

	if (end - start <= 1)
	{
		tree->mLeft = nullptr;
		tree->mRight = nullptr;
		return;
	}
	tree->mLeft = new BvNode();
	tree->mRight = new BvNode();

	int mid = (start + end) / 2;

	tree->mLeft->mDataStart = start;
	tree->mLeft->mDataEnd = mid;

	tree->mRight->mDataStart = mid;
	tree->mRight->mDataEnd = end;

	tree->mLeft->mMaxCoord = dataBuffer[tree->mLeft->mDataStart].MaxCoord;
	tree->mLeft->mMinCoord = dataBuffer[tree->mLeft->mDataStart].MinCoord;

	for (int i = tree->mLeft->mDataStart; i < tree->mLeft->mDataEnd; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			tree->mLeft->mMaxCoord.m128_f32[j] = (tree->mLeft->mMaxCoord.m128_f32[j] < dataBuffer[i].MaxCoord.m128_f32[j]) ?
				dataBuffer[i].MaxCoord.m128_f32[j] : tree->mLeft->mMaxCoord.m128_f32[j];

			tree->mLeft->mMinCoord.m128_f32[j] = (tree->mLeft->mMinCoord.m128_f32[j] > dataBuffer[i].MinCoord.m128_f32[j]) ?
				dataBuffer[i].MinCoord.m128_f32[j] : tree->mLeft->mMinCoord.m128_f32[j];
		}
	}

	tree->mRight->mMaxCoord = dataBuffer[tree->mRight->mDataStart].MaxCoord;
	tree->mRight->mMinCoord = dataBuffer[tree->mRight->mDataStart].MinCoord;
	for (int i = tree->mRight->mDataStart; i < tree->mRight->mDataEnd; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			
			tree->mRight->mMaxCoord.m128_f32[j] = (tree->mRight->mMaxCoord.m128_f32[j] < dataBuffer[i].MaxCoord.m128_f32[j]) ?
				dataBuffer[i].MaxCoord.m128_f32[j] : tree->mRight->mMaxCoord.m128_f32[j];

			tree->mRight->mMinCoord.m128_f32[j] = (tree->mRight->mMinCoord.m128_f32[j] > dataBuffer[i].MinCoord.m128_f32[j]) ?
				dataBuffer[i].MinCoord.m128_f32[j] : tree->mRight->mMinCoord.m128_f32[j];
		}
	}

	BuildBVTree(dataBuffer,tree->mLeft, tree->mLeft->mDataStart, tree->mLeft->mDataEnd);
	BuildBVTree(dataBuffer,tree->mRight, tree->mRight->mDataStart, tree->mRight->mDataEnd);
}

void MapObject::MapBuild()
{
	Partition(mMapDataBuffer,0, mMapDataBufferCount, AXIS_X);
	BuildBVTree(mMapDataBuffer,mBoxVolumeTree ,0 , mMapDataBufferCount);
}

float Map::ComputePenetration(SWEEP_HIT_T& swpHit,CAPSULE_T& capsule)
{
	float radius = capsule.radius;
	XMVECTOR axis = capsule.head - capsule.foot;
	XMVECTOR point = swpHit.point - capsule.foot;
	float distace = 0;
	
	if (XMVector3Dot(point, axis).m128_f32[0] < 0)
	{
		distace = XMVector3Length(point).m128_f32[0];
	}
	else if (XMVector3Dot(point, axis).m128_f32[0] > XMVector3Dot(axis, axis).m128_f32[0])
	{
		distace = XMVector3Length(swpHit.point - capsule.head).m128_f32[0];
	}
	else
	{
		distace = XMVector3LinePointDistance(capsule.head, capsule.foot, swpHit.point).m128_f32[0];
	}
	return radius - distace;

}



#include <stdio.h>

void SearchBVTreeSwpCollision(COLLISION_INFO_T* maxCollisionInfo,int* firstCol,int depth , XMVECTOR point,TRIPOLY_T* map,BvNode* tree,bool* result, CapsuleCollider& player, CAPSULE nextCapsule, eastl::vector<SWEEP_HIT_T>& outSwpSet)
{
	
	if (*result) return;
	if (tree == nullptr) return;

	float margin = player.Collider.radius + 1.5;
	XMVECTOR maxCoord = tree->mMaxCoord;
	XMVECTOR minCoord = tree->mMinCoord;

	for (int i = 0; i < 3; i++)
	{
		maxCoord.m128_f32[i] += margin;
		minCoord.m128_f32[i] -= margin;
	}
	
	if (player.BoxTest(maxCoord, minCoord))
	{
		if((tree->mLeft == nullptr && tree->mRight == nullptr) || depth <= 0)
		{
			
			for (int i = tree->mDataStart; i < tree->mDataEnd; i++)
			{
				
				XMVECTOR rayPoint;
				XMVECTOR normal;
				VERTEX_T temp[3] = {
					{map[i].Vertex[0] ,},
					{map[i].Vertex[1] ,},
					{map[i].Vertex[2] ,}
				};
				XMVECTOR triVector[3] = {
					{XMLoadFloat4(&map[i].Vertex[0]) },
					{XMLoadFloat4(&map[i].Vertex[1]) },
					{XMLoadFloat4(&map[i].Vertex[2]) },
				};

				SWEEP_HIT_T swpInfo;
				swpInfo.element = -1;
				if (player.TriAngleCollisionTest(triVector, nextCapsule, &swpInfo) && swpInfo.element != -1)
				{	

					swpInfo.penetrationDepth = ComputePenetration(swpInfo, nextCapsule);
					outSwpSet.push_back(swpInfo);
					//printf("push %d %f\n", outSwpSet.size() , swpInfo.penetrationDepth);
		
				}

			}
			
		}

		
		if (depth > 0)
		{
			SearchBVTreeSwpCollision(maxCollisionInfo, firstCol, depth - 1, point, map, tree->mLeft, result, player, nextCapsule, outSwpSet);
			SearchBVTreeSwpCollision(maxCollisionInfo, firstCol, depth - 1, point, map, tree->mRight, result, player, nextCapsule, outSwpSet);
		}
		
	}
	else
	{
		return;
	}

	
	
}


bool MapObject::IsMapSwpCollisionDetect(CapsuleCollider& player,CAPSULE nextCapsule, eastl::vector<SWEEP_HIT_T>& outSwpSet)
{
	XMVECTOR searchPoint = (player.Collider.head - player.Collider.foot)/2;

	XMVECTOR rayPoint;
	XMVECTOR normal;

	int partitionCurStart = 0;
	int partitionCurEnd = mMapDataBufferCount;
	
	bool result = false;
	
	COLLISION_INFO_T collision = {
			0,
	};
	int firstCollision = 0;



	SearchBVTreeSwpCollision(&collision,&firstCollision, 14 , player.Collider.head, mMapDataBuffer, mBoxVolumeTree, &result , player, nextCapsule , outSwpSet);

	SearchBVTreeSwpCollision(&collision, &firstCollision,14 , player.Collider.foot, mMapDataBuffer, mBoxVolumeTree, &result, player, nextCapsule, outSwpSet);
	//printf("size : %d \n", swpSet.size());


	if (outSwpSet.empty()) {
		return false; // 충돌 없음
	}
	
	/**/
	eastl ::sort(outSwpSet.begin(), outSwpSet.end(),
		[](const SWEEP_HIT_T& a, const SWEEP_HIT_T& b) {
			return a.hitTime < b.hitTime; // 충돌 시점으로 정렬
		});
	

	return true; // 충돌이 발생했음을 알림

}