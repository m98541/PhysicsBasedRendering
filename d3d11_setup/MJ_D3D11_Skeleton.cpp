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

	this->headJointId = 0;// FindHeadJoindIndex(); 일단 root 로 고정
	// 헤드에 카메라를 우겨 넣는게 아닌 캡슐 을 감싸고 해당 캡슐의 3:7부분에 카메라 위치 후 1인칭 모드에서 케릭터 렌더링 안하는 방식으로 변경
	// 일단 root에 카메로 고정후 캡슐 collider 캐릭터 연동 후 캐릭터 <-> 카메라 <-> 맵 상호작용먼저 개발
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


uint32_t Skeleton::FindHeadJoindIndex()
{
	eastl::vector<eastl::string> headNameList = {"HEAD","Head","head"};

	for (int i = 0; i < jointsData->count; i++)
	{
		for (int j = 0; j < headNameList.size(); j++)
		{
			printf("%s \n", jointsData->array[i].jointName.c_str());
			if ( jointsData->array[i].jointName.find(headNameList[j].c_str()) != eastl::string::npos ) 
			{
				return i;
			}
		}

	}
	return 0;
}

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

DirectX::XMFLOAT4X4 Skeleton::GetGlobalHeadJointPoseMat()
{
	return this->pose.jointsGlobalPoseArr[headJointId];
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