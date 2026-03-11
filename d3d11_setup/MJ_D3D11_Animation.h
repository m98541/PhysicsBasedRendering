#ifndef MJ_D3D11_ANIMATION_H
#define MJ_D3D11_ANIMATION_H

#include "MJ_D3D11_Skeleton.h"
#include <EASTL/vector.h>


typedef struct AnimationKeyTrans_S
{
	float time;
	DirectX::XMFLOAT3 trans;
}AnimationKeyTrans_T;

typedef struct AnimationKeyQuatRot_S
{
	float time;
	DirectX::XMFLOAT4 quatRot;
}AnimationKeyQuatRot_T;

typedef struct AnimationKeyScale_S
{
	float time;
	DirectX::XMFLOAT3 scale;
}AnimationKeyScale_T;

typedef struct AnimationResource_S AnimationResource_T;
typedef struct AnimationClipResourec_S AnimationClipResourec_T;

typedef struct AnimationJoint_S// 에니메이션의 키포즈가 되어짐 
{
	uint32_t localPoseArrIndex;

	uint32_t lastTransKeyIndex;
	uint32_t lastQuatKeyIndex;
	uint32_t lastScaleKeyIndex;

	eastl::vector<AnimationKeyTrans_S> transKeys;
	eastl::vector<AnimationKeyQuatRot_S> quatRotKeys;
	eastl::vector<AnimationKeyScale_S> scaleKeys;

}AnimationJoint_T;

class AnimationManager
{
	private:
		uint32_t curAnimation;
		const float* globalTime;
		float localTime;
		Skeleton* targetSkeleton;
		AnimationResource_T* animationResource;
		void SkeletonDataUpdate(AnimationClipResourec_T* curAnimationClip, float t);

	public:
		AnimationManager();
		AnimationManager(Skeleton* targetSkeleton ,AnimationResource_T* animationResource);
		~AnimationManager();
		
		
		void NextAnimation();
		void SelectAnimation(uint32_t animationId);
		void NextTimePose(float deltaT);
};


#endif // !MJ_D3D11_ANIMATION_H
