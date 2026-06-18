#include "MJ_D3D11_Animation.h"
#include "MJ_D3D11_CharacterResource.h"


AnimationManager::AnimationManager()
{
	curAnimation = 0;
	globalTime = nullptr;
	localTime = 0;
	targetSkeleton = nullptr;
}
 
AnimationManager::~AnimationManager()
{

}

AnimationManager::AnimationManager(Skeleton* targetSkeleton, AnimationResource_T* animationResource)
{
	this->targetSkeleton = targetSkeleton;
	this->animationResource = animationResource;
	this->globalTime = globalTime;
	curAnimation = 0;
	localTime = 0;

	for (uint32_t i = 0; i < animationResource->count; ++i)
	{
		for (uint32_t j = 0; j <targetSkeleton->jointsData->count; ++j)
		{
			animationResource->animationClip[i].AnimationJoint[j].lastQuatKeyIndex = 0;
			animationResource->animationClip[i].AnimationJoint[j].lastScaleKeyIndex = 0;
			animationResource->animationClip[i].AnimationJoint[j].lastTransKeyIndex = 0;
		}
	}
}

void AnimationManager::NextAnimation()
{
	curAnimation = (curAnimation++) % animationResource->count;
}

void AnimationManager::SelectAnimation(uint32_t animationId)
{
	if (animationId > animationResource->count - 1)
	{
		curAnimation = 0;
	}
	else
	{
		if (curAnimation != animationId)
			this->localTime = 0;
		curAnimation = animationId;
	}
}

void AnimationManager::NextTimePose(float deltaT)
{
	localTime = (localTime + deltaT);
	

	float clipTime = animationResource->animationClip[curAnimation].totalTime;
	
	//float 모듈러 없어서 걍 넘으면 초기화임 일단
	if (localTime > clipTime) localTime = deltaT;

	float nomalTime = localTime / clipTime;

	SkeletonDataUpdate(animationResource->animationClip + curAnimation, nomalTime );
}

// t -> 0~1 의 값으로 넘겨야됨
/*
즉 밑의 함수 외부(즉 AnimationManager가)에서 해당 클립의 total time을 가져가서 t = (local_time / total_time) 하여금 만들어 줘야함
즉 clip 시작시 local_time 초기화 필요 local_time = (local_time + (cur_global_time - last_global_time)) % total_time  다음 처럼 구성하면 될 듯
clip Init 이 필요함 -> clip init에서 이전 clip의 타임라인 인덱스 의 초기화도 필요함
*/

//즉 해당 매서드의 역할은 특정 클립의 t(0~1)시점의 동작으로 로컬 포즈를 잡아주는 역할만을 수행하게 함.
void AnimationManager::SkeletonDataUpdate(AnimationClipResourec_T* curAnimationClip  , float t)
{
	
	uint32_t searchTimeLineTransIndex = 0;
	uint32_t searchTimeLineQuatRotIndex = 0;
	uint32_t searchTimeLineScaleIndex = 0;

	// 정규화된 상태에서의 구간별 간격 {start , end}
	DirectX::XMFLOAT2 transInterVal = { 0 , 0 };
	DirectX::XMFLOAT2 quatInterVal = { 0 , 0 };
	DirectX::XMFLOAT2 scaleInterVal = { 0 , 0 };

	//정규화 된 보간값 무조건 0~1 에 위치해야함
	// debug를 위한 assert 필요
	float transInterpCoeffs = 0;
	float quatInterpCoeffs = 0;
	float scaleInterpCoeffs = 0;

	//일단은 무조건 선형 보간 사용 이후 스플라인 보간법 사용 예정
	//관절마다 타임 라인 구획이 다르므로 
	// 관절 탐색하면서 적용해줘야함
	for (uint32_t i = 0; i < targetSkeleton->jointsData->count ; ++i)
	{
		searchTimeLineQuatRotIndex = curAnimationClip->AnimationJoint[i].lastQuatKeyIndex;
		searchTimeLineTransIndex = curAnimationClip->AnimationJoint[i].lastTransKeyIndex;
		searchTimeLineScaleIndex = curAnimationClip->AnimationJoint[i].lastScaleKeyIndex;

		
		uint32_t quatKeySize = curAnimationClip->AnimationJoint[i].quatRotKeys.size();
		uint32_t transKeySize = curAnimationClip->AnimationJoint[i].transKeys.size();
		uint32_t scaleKeySize = curAnimationClip->AnimationJoint[i].scaleKeys.size();
		
		if (quatKeySize > 0)
		{

			float totalTime = curAnimationClip->AnimationJoint[i].jointQuatDurationTime;
			// quat 구간 탐색
			for (uint32_t j = searchTimeLineQuatRotIndex; j < quatKeySize; ++j)
			{

				quatInterVal.x = curAnimationClip->AnimationJoint[i].quatRotKeys[j].time / totalTime;
				quatInterVal.y = curAnimationClip->AnimationJoint[i].quatRotKeys[(j + 1) % (quatKeySize)].time / totalTime;
				if (t >= quatInterVal.x && t <= quatInterVal.y)
				{
					searchTimeLineQuatRotIndex = (searchTimeLineQuatRotIndex + j) % (quatKeySize);
					break;
				}
			}

			if (quatKeySize == 1)
			{
				DirectX::XMVECTOR quat0 = DirectX::XMLoadFloat4(&curAnimationClip->AnimationJoint[i].quatRotKeys[searchTimeLineQuatRotIndex].quatRot);
				DirectX::XMStoreFloat4(&targetSkeleton->jointsData->array[i].jointLocalPose.quatRot, quat0);
			}
			else
			{
				quatInterpCoeffs = (t - quatInterVal.x) / (quatInterVal.y - quatInterVal.x);

				if (quatInterpCoeffs > 1 || quatInterpCoeffs < 0)
					assert(false && "calc quatInterpCoeffs fail");
				DirectX::XMVECTOR quat0 = DirectX::XMLoadFloat4(&curAnimationClip->AnimationJoint[i].quatRotKeys[searchTimeLineQuatRotIndex].quatRot);
				DirectX::XMVECTOR quat1 = DirectX::XMLoadFloat4(&curAnimationClip->AnimationJoint[i].quatRotKeys[(searchTimeLineQuatRotIndex + 1) % quatKeySize].quatRot);
			
				DirectX::XMVECTOR curQuatRot =DirectX::XMQuaternionSlerp(quat0, quat1, quatInterpCoeffs);
				curQuatRot = DirectX::XMQuaternionNormalize(curQuatRot);

				DirectX::XMStoreFloat4(&targetSkeleton->jointsData->array[i].jointLocalPose.quatRot, curQuatRot);
			}
			

		}
		
		

		if (transKeySize > 0)
		{

			float totalTime = curAnimationClip->AnimationJoint[i].jointTransDurationTime;
			// tran 구간 탐색
			for (uint32_t j = searchTimeLineTransIndex; j < transKeySize; ++j)
			{
				transInterVal.x = curAnimationClip->AnimationJoint[i].transKeys[j].time / totalTime;
				transInterVal.y = curAnimationClip->AnimationJoint[i].transKeys[(j + 1) % (transKeySize)].time / totalTime;

				if (t >= transInterVal.x && t <= transInterVal.y)
				{
					searchTimeLineTransIndex = (searchTimeLineTransIndex + j) % (transKeySize);
					break;
				}
			}
		


			if (transKeySize == 1)
			{
				//targetSkeleton->jointsData 를 직접 수정하는것이 맞는 선택일까....?
			
				DirectX::XMVECTOR trans0 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].transKeys[searchTimeLineTransIndex].trans);
		
				DirectX::XMStoreFloat3(&targetSkeleton->jointsData->array[i].jointLocalPose.trans, trans0);
			}
			else
			{
				transInterpCoeffs = (t - transInterVal.x) / (transInterVal.y - transInterVal.x);
				if (transInterpCoeffs > 1 || transInterpCoeffs < 0)
					assert(false && "calc transInterpCoeffs fail");

				//targetSkeleton->jointsData 를 직접 수정하는것이 맞는 선택일까....?
				DirectX::XMVECTOR trans0 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].transKeys[searchTimeLineTransIndex].trans);
				DirectX::XMVECTOR trans1 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].transKeys[(searchTimeLineTransIndex + 1) % (transKeySize)].trans);
				trans0 = DirectX::XMVectorScale(trans0, (1 - transInterpCoeffs));
				trans1 = DirectX::XMVectorScale(trans1, (transInterpCoeffs));
				DirectX::XMStoreFloat3(&targetSkeleton->jointsData->array[i].jointLocalPose.trans, DirectX::XMVectorAdd(trans0, trans1));
			}
			

		}
		
	
		if (scaleKeySize > 1)
		{

			float totalTime = curAnimationClip->AnimationJoint[i].jointScaleDurationTime;
			// scale 구간 탐색
			for (uint32_t j = searchTimeLineScaleIndex; j < scaleKeySize; ++j)
			{
				scaleInterVal.x = curAnimationClip->AnimationJoint[i].scaleKeys[j].time / totalTime;
				scaleInterVal.y = curAnimationClip->AnimationJoint[i].scaleKeys[(j + 1) % scaleKeySize].time / totalTime;

				if (t >= scaleInterVal.x && t <= scaleInterVal.y)
				{
					searchTimeLineScaleIndex = (searchTimeLineScaleIndex + j) % (scaleKeySize);
					break;
				}
			}

			if (scaleKeySize == 1)
			{

				printf("\n scaleKeySize one!! \n");
				DirectX::XMVECTOR scale0 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].scaleKeys[searchTimeLineScaleIndex].scale);
				DirectX::XMStoreFloat(&targetSkeleton->jointsData->array[i].jointLocalPose.scale, scale0);

			}
			else
			{

				scaleInterpCoeffs = (t - scaleInterVal.x) / (scaleInterVal.y - scaleInterVal.x);
				if (scaleInterpCoeffs > 1 || scaleInterpCoeffs < 0)
					assert(false && "calc transInterpCoeffs fail");

				DirectX::XMVECTOR scale0 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].scaleKeys[searchTimeLineScaleIndex].scale);
				DirectX::XMVECTOR scale1 = DirectX::XMLoadFloat3(&curAnimationClip->AnimationJoint[i].scaleKeys[(searchTimeLineScaleIndex + 1) % scaleKeySize].scale);
				scale0 = DirectX::XMVectorScale(scale0, (1 - scaleInterpCoeffs));
				scale1 = DirectX::XMVectorScale(scale1, (scaleInterpCoeffs));
				DirectX::XMStoreFloat(&targetSkeleton->jointsData->array[i].jointLocalPose.scale, DirectX::XMVectorAdd(scale0, scale1));

			}

		}

	}


	targetSkeleton->Update();
	
}
