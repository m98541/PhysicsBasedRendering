#ifndef MJ_D3D11_SKELETON_H
#define MJ_D3D11_SKELETON_H
#include <DirectXMath.h>
#include <string>

typedef struct Joint_S Joint_T;
typedef struct SkeletonResource_S SkeletonResource_T;
typedef struct JointPose_S JointPose_T;
typedef struct SkeletonPose_S SkeletonPose_T;


typedef struct JointPose_S
{
	DirectX::XMFLOAT4 quatRot;
	DirectX::XMFLOAT3 trans;
	float scale;

}JointPose_T;

typedef struct Joint_S
{
	DirectX::XMFLOAT4X4 inverseBindPose;
	unsigned int parentIndex;
	std::string jointName;
	std::string nodeId;
	JointPose_T jointLocalPose;
}Joint_T;

typedef struct SkeletonResource_S
{
	unsigned int count;
	Joint_T* array;
}SkeletonResource_T;



typedef struct SkeletonPose_S
{
	unsigned int count;
	DirectX::XMFLOAT4X4* jointsLocalPoseArr;
	DirectX::XMFLOAT4X4* jointsGlobalPoseArr;

}SkeletonPose_T;

class Skeleton {
	// 포즈에 대한 실시간 업데이트 필요 Update 함수 통해 , 생성자는 그저 초기화 요소
	public:
		Skeleton();
		Skeleton(SkeletonResource_T* jointsData);
		~Skeleton();
		void Update(float deltaTime);
		SkeletonPose_T pose;
	private:
		void JointsDataSort();
		void JointsLocalInit();
		void JointsGlobalPoseCompute();

		SkeletonResource_T* jointsData;
		
		DirectX::XMFLOAT4X4 wordTransMat;
	
};

#endif // !MJ_D3D11_SKELETON_H
