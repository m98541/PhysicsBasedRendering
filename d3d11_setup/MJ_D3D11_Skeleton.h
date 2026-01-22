#ifndef MJ_D3D11_SKELETON_H
#define MJ_D3D11_SKELETON_H
#include <DirectXMath.h>
#include <string>
typedef struct Joint_S
{
	DirectX::XMFLOAT4X4 inverseBindPose;
	unsigned int parentIndex;
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
	bool* computedMatrixArr;// 동적 프로그래밍 기법 통해 자식 에서 부모로 곱 일어날 때 중복 연산 제거 
	JointPose_T* jointLocalPoseArr;
	DirectX::XMFLOAT4X4* jointGlobalPoseArr;
	
}SkeletonPose_T;

class Skeleton {
	// 포즈에 대한 실시간 업데이트 필요 Update 함수 통해 , 생성자는 그저 초기화 요소
	public:
		Skeleton();
		Skeleton(SkeletonResource_T* jointsData);
		~Skeleton();
		void Update(float deltaTime);
	private:
		void jointsDataSort();
		SkeletonResource_T* jointsData;
		SkeletonPose_T pose;
		DirectX::XMFLOAT4X4 wordTransMat;
	
};

#endif // !MJ_D3D11_SKELETON_H
