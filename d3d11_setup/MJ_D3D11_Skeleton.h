#ifndef MJ_D3D11_SKELETON_H
#define MJ_D3D11_SKELETON_H
#include <DirectXMath.h>
#include <string>
#include <EASTL/map.h>
#include <EASTL/string.h>
#include <EASTL/queue.h>
#include <EASTL/vector.h>
typedef struct Joint_S Joint_T;
typedef struct JointPose_S JointPose_T;
typedef struct SkeletonPose_S SkeletonPose_T;
typedef struct SkeletonResource_S  SkeletonResource_T;

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
	std::string jointId;
	JointPose_T jointLocalPose;
}Joint_T;



typedef struct SkeletonPose_S
{
	unsigned int count;
	DirectX::XMFLOAT4X4* jointsLocalPoseArr;
	DirectX::XMFLOAT4X4* jointsGlobalPoseArr;
	DirectX::XMFLOAT4X4* jointsInversePoseArr;
}SkeletonPose_T;

class Skeleton {
	// 포즈에 대한 실시간 업데이트 필요 Update 함수 통해 , 생성자는 그저 초기화 요소
	// 현재(26.02.21) 키프레임 시스템 도입 전으로 포즈 여러 개를 미리 구워 두고 이를 교체해가며 사용할 예정
	// 런타임 중에 포즈를 init 하는 건 말도 안됨
	// 포즈에 대한 등록 ,인덱스 부여 , 포즈 교체등의 함수 필요
	// 스켈레톤 포즈 또한 벡터형태로 교체 예정 그리고 포즈 내에 사람이 읽을 수 있는 pose name 부여 예정
	// pose의 경우 name(key) , pose(value) 형태로 관리 , name list 를 통해 개발자가 해당 스켈레톤의 
	// name list 를 보고 name(key)를 통해 pose를 호출할 수 있도록 할 예정
	// pose 의 경우 pose register 를 통해 map 에 등록 시키고 무결성 검사(입력 포즈의 관절 수 <-> 캐릭터 관절 수 검사 , 현재(26.02.21)는 일단 최소한의 검사)
	// pose update 함수 를 통해 pose를 업데이트 해줌
	public:
		Skeleton();
		Skeleton(SkeletonResource_T* jointsData);
		~Skeleton();
		void Update();

		const DirectX::XMFLOAT4X4* GetGlobalJoints();
		const DirectX::XMFLOAT4X4* GetInverseJoints();
		DirectX::XMFLOAT4X4 GetGlobalHeadJointPoseMat();
		uint32_t GetGlobalJointsCount();

		SkeletonResource_T* jointsData;
		SkeletonPose_T pose;// curPose


	//	void PoseUpdate(std::string keyPoseName); 일단 기본 포즈 렌더링 세팅 후 도입 
	//	void PoseRegister(std::string keyPoseName ,SkeletonPose_T& pose);

	private:
		//void JointsDataSort(); loader로 이전됨
		void JointsLocalUpdate();
		void JointsGlobalPoseCompute();

		// head 부분 인덱스 탐색 -> 이후 해당 부분의 좌표 기준으로 카메시점 변환에 사용 예정
		uint32_t FindHeadJoindIndex();
		uint32_t headJointId;

		uint32_t poseCount;
		
		DirectX::XMFLOAT4X4 wordTransMat;
		
	//	eastl::vector<eastl::string> nameList;
	//	eastl::map<eastl::string, SkeletonPose_T >poseSet;

	
};

#endif // !MJ_D3D11_SKELETON_H
