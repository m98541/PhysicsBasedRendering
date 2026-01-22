#include <EASTL/queue.h>
#include <EASTL/vector.h>


#include "MJ_D3D11_Skeleton.h"

using namespace DirectX;

Skeleton::Skeleton()
{

}

//Skeleton 생성자임 절대로 업데이트의 대용(특히 loop 내에서)으로 사용 금지
Skeleton::Skeleton(SkeletonResource_T* jointsData)
{
	this->jointsData = jointsData;
	this->pose.count = this->jointsData->count; 
	this->jointsDataSort();// 데이터 정규화 , 부모-자식간 선 후 순위 보장 , 및 행렬 전치

}

Skeleton::~Skeleton()
{

}

typedef struct SortBucket_S
{
	int originIndex;
	int updateParent;
}SortBucket_T;


//리소스의 부모 - 자식 관계 순서 보장 원본 리소스 수정
void Skeleton::jointsDataSort()
{
	//init
	int maxCount = this->jointsData->count;
	eastl::queue<SortBucket_T> orderQueue;
	Joint_T* resultBuffer = new Joint_T[maxCount];
	int searchParent = 0;
	int updateParent = 0;
	//loop
	int OrgBufferRootMin = 0;
	int bufferSizeCount = 0;

	while (bufferSizeCount != maxCount)
	{
		//root 찾기 Origin 데이터에서 현재 index 와 부모 index 동일시 root 로 간주함
		int root = OrgBufferRootMin;
		for (root; 
			root < maxCount && 
			root != this->jointsData->array[root].parentIndex && 
			root <= OrgBufferRootMin; 
			root++);

		if (root > OrgBufferRootMin) OrgBufferRootMin = root;

		SortBucket_T rootJoint = { root  ,bufferSizeCount };
		orderQueue.push(rootJoint);

		//root 하위영역 찾기
		while (!orderQueue.empty())// 큐가 비었다 leap 까지 모두 도달 but  bufferSizeCount != maxCount 면 root 여러개
		{
			SortBucket_T reciveBucket = orderQueue.front();
			orderQueue.pop();

			resultBuffer[bufferSizeCount] = this->jointsData->array[reciveBucket.originIndex];
			resultBuffer[bufferSizeCount].parentIndex = reciveBucket.updateParent;

			XMMATRIX transposeMat4X4 = XMLoadFloat4x4(&resultBuffer[bufferSizeCount].inverseBindPose);
			transposeMat4X4 = XMMatrixTranspose(transposeMat4X4);
			XMStoreFloat4x4(&resultBuffer[bufferSizeCount].inverseBindPose, transposeMat4X4);

			updateParent = bufferSizeCount++;

			searchParent = reciveBucket.originIndex;
			for (int i = 0; i < maxCount; ++i)
			{
				if (i != jointsData->array[i].parentIndex &&
					searchParent == jointsData->array[i].parentIndex)
				{
					SortBucket_T sendBucket = { i  , updateParent};
					orderQueue.push(sendBucket);
				}
			}
		}

		
	}
	//정렬된 데이터 셋 기존 데이터와 교체
	memcpy(this->jointsData->array, resultBuffer , this->jointsData->count * sizeof(Joint_T));

	
	
}