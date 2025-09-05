
#ifndef BVTREE_H
#define BVTREE_H

#include "VertexType.h"

namespace BVtree
{
	class BvNode
	{
	public:
		BvNode();
		~BvNode();
		bool IsPointInBox(DirectX::XMVECTOR point);

		DirectX::XMVECTOR mMaxCoord;
		DirectX::XMVECTOR mMinCoord;
		
		struct BvNode* mLeft;
		struct BvNode* mRight;
		int mDataStart;
		int mDataEnd;

	};
};

#endif // !BVTREE_H
