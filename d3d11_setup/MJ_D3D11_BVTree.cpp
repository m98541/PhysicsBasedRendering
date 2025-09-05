#include "MJ_D3D11_BVTree.h"

using namespace BVtree;
using namespace DirectX;

BvNode::BvNode()
{
	mMaxCoord = {0 , 0 , 0 , 1.F};
	mMinCoord = {0 , 0 , 0 , 1.F };
	mLeft = nullptr;
	mRight = nullptr;
	mDataStart = 0;
	mDataEnd = 0;
}

BvNode::~BvNode()
{
	if(mLeft != nullptr)
		mLeft->~BvNode();

	if (mRight != nullptr)
		mRight->~BvNode();
}

bool BvNode::IsPointInBox(DirectX::XMVECTOR point)
{
	return  (point.m128_f32[0] <= mMaxCoord.m128_f32[0] && point.m128_f32[1] <= mMaxCoord.m128_f32[1] && point.m128_f32[2] <= mMaxCoord.m128_f32[2]) &&
			(point.m128_f32[0] >= mMinCoord.m128_f32[0] && point.m128_f32[1] >= mMinCoord.m128_f32[1] && point.m128_f32[2] >= mMinCoord.m128_f32[2]);
}

