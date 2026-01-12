#ifndef MJ_D3D11_SKELETON_H
#define MJ_D3D11_SKELETON_H
#include <DirectXMath.h>
#include <string>
typedef struct Joint_S
{
	DirectX::XMFLOAT4X4 inverseBindPose;
	unsigned int parentIndex;
	unsigned int childrenCount;
	unsigned int* childrenIndices;
	std::string jointName;
}Joint_T;

typedef struct SkeletonResource_S
{
	unsigned int count;
	Joint_T* array;
}SkeletonResource_T;

typedef struct JointPose_S
{
	DirectX::XMFLOAT4 quatRot;
	DirectX::XMFLOAT3 trans;
	float scale;
}JointPose_T;

typedef struct SkeletonPose_S
{
	unsigned int count;
	JointPose_T* jointLocalPoseArr;
	DirectX::XMFLOAT4X4* jointGlobalPoseArr;
	
}SkeletonPose_T;

class Skeleton {

	public:
		Skeleton(SkeletonResource_T* jointsData);
		~Skeleton();
		void Update(float deltaTime);
	private:
		SkeletonResource_T* jointsData;
		SkeletonPose_T pose;
		DirectX::XMFLOAT4X4 wordTransMat;
	
};

#endif // !MJ_D3D11_SKELETON_H
