#include "MJ_D3D11_Skeleton.h"
#include "MJ_D3D11_CharacterResource.h"

using namespace DirectX;

Skeleton::Skeleton()
{

}

//Skeleton 생성자임 절대로 업데이트의 대용(특히 loop 내에서)으로 사용 금지
Skeleton::Skeleton(SkeletonResource_T* jointsData)
{

	this->jointsData = jointsData;
	this->pose.count = this->jointsData->count;

	this->pose.jointsLocalPoseArr = new XMFLOAT4X4[this->pose.count];
	this->pose.jointsGlobalPoseArr = new XMFLOAT4X4[this->pose.count];
	this->pose.jointsInversePoseArr = new XMFLOAT4X4[this->pose.count];

	Skeleton::Update();
	//this->JointsDataSort();// 데이터 정규화 , 부모-자식간 선 후 순위 보장 , 및 행렬 전치 , // 리소스에서 정규화 보장되어야함

}

Skeleton::~Skeleton()
{

}

typedef struct SortBucket_S
{
	int originIndex;
	int updateParent;
}SortBucket_T;


void Skeleton::Update()
{
	this->JointsLocalUpdate();

	this->JointsGlobalPoseCompute();
}

// 2026.02.02 JointsDataSort 기능 -> GLTFLoader 로 이전/
// 이전 완료 및 character 완료시 해당 파일에서는 삭제 예정
// 데이터 정렬은 Loader 에서 해결이 되어야함 Skeleton 은 정렬이 보장된 데이터 만을 받아야함
// 정렬 체크만 지원하여 정렬 안되있음면 error 발생
//리소스의 부모 - 자식 관계 순서 보장 원본 리소스 수정 
/*
void Skeleton::JointsDataSort()
{
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
*/

//pose quat rot , trans , scale 행렬 변환


void Skeleton::JointsLocalUpdate()
{

	XMVECTOR trans;
	XMVECTOR quatRot;
	XMVECTOR scale;
	for (int i = 0; i < this->jointsData->count; i++)
	{
		trans = XMLoadFloat3( &this->jointsData->array[i].jointLocalPose.trans);
		XMVectorSetW(trans, 1.F);

		quatRot = XMLoadFloat4(&this->jointsData->array[i].jointLocalPose.quatRot);

		scale = XMVectorReplicate(this->jointsData->array[i].jointLocalPose.scale);
		XMVectorSetW(scale, 1.F);
	
	

		XMStoreFloat4x4(this->pose.jointsLocalPoseArr + i ,XMMatrixAffineTransformation(scale,g_XMZero,quatRot,trans) );
		this->pose.jointsInversePoseArr[i] = jointsData->array[i].inverseBindPose;
	}
}

void Skeleton::JointsGlobalPoseCompute()// 
{
	XMMATRIX xmCurLocalMat;
	XMMATRIX xmGlobalParentMat;
	for (int i = 0; i < this->pose.count; i++)
	{
		if (i == this->jointsData->array[i].parentIndex)
		{//root 인 경우
			this->pose.jointsGlobalPoseArr[i] = this->pose.jointsLocalPoseArr[i];
		}
		else
		{//root 아닌 경우 , 부모 joint 무조건 배열 앞 임으로 자식 글로벌 = 자식 로컬 * 부모 글로벌
			xmCurLocalMat = XMLoadFloat4x4(&this->pose.jointsLocalPoseArr[i]);
			xmGlobalParentMat = XMLoadFloat4x4(&this->pose.jointsGlobalPoseArr[this->jointsData->array[i].parentIndex]);
			XMStoreFloat4x4(this->pose.jointsGlobalPoseArr + i ,xmCurLocalMat * xmGlobalParentMat );
		}
	}


}

const DirectX::XMFLOAT4X4* Skeleton::GetGlobalJoints()
{
	JointsGlobalPoseCompute();
	return this->pose.jointsGlobalPoseArr;
}


const DirectX::XMFLOAT4X4* Skeleton::GetInverseJoints()
{
	return this->pose.jointsInversePoseArr;
}

uint32_t Skeleton::GetGlobalJointsCount()
{
	return this->pose.count;
}