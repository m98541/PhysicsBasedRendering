#ifndef MJ_D3D11_CHARACTER_H
#define MJ_D3D11_CHARACTER_H
#define NOMINMAX
#include <DirectXMath.h>
#include "MJ_D3D11_CharacterResource.h"
#include "MJ_D3D11_Skeleton.h"
#include "MJ_D3D11_Mesh.h"
#include "MJ_D3D11_Animation.h"
#include "MS_GLTFLoader.h"
#include "MJ_D3D11_BasicCam.h"

// 캐릭터 데이터에 대한 소유 및 상태 변화 를 책임 렌더러에 필요데이터 전달해줌 , 전달 데이터에 대한 책임을 짐


/*
	5월 21일 목
	캐릭터 헤드 카메라 시스템 개발 시작
	1.헤드캠의 기본 pos : 항상 0,0,0 -> 변환 과정 : 먼저 스켈레톤 헤드 행렬 곱 이후 캐릭터의 pos 행렬 곱

	2.헤드캠이 바라보는 방향과 캐릭터의 방향은 서로 다를 수 있음 즉 캐릭터의 방향위에서 헤드캠의 방향 결정 
	
	1,2 해보니 아닌거 같음 막흔들리고 눈깔보이고 난리남..

	5월 22일 금
	헤드에 카메라를 우겨 넣는게 아닌 캡슐 을 감싸고 해당 캡슐의 3:7부분에 카메라 위치 후 1인칭 모드에서 케릭터 렌더링 안하는 방식으로 변경
	일단 root에 카메로 고정후 캡슐 collider 캐릭터 연동 후 캐릭터 <-> 카메라 <-> 맵 상호작용먼저 개발

*/
class Character
{
	public:
		void LoadResource(CharacterResource* characterResource);
		Character();
		Character(CharacterResource* characterResource);
		~Character();

		// 캐릭터의 조인트 정보 등을 업데이트 , gpu 에 해당 내용 반영을 위해서는 캐릭터 업데이트 이후 캐릭터 렌더러 또한 업데이트 해주어야 함
		// 캐릭터의 메모리의 행렬 정보 업데이트 현재는 캐릭터의 현재 정보만을 없데이트 해주는 방식
		// 이후에 키프레임 방식으로 개선이 필요함
		// 일단 여러 포즈를 미리 구워두고 이를 교체하는 방식으로 사용!
		void updateAnimation(uint32_t animationId ,float deltaT);
		void updateHeadCam(BasicCam* outCamData);

		const MeshInfo* GetMeshInfo() const;
		const Skeleton* GetSkeleton() const;
		const TextureResourec_T* GetTextureResource() const;
		DirectX::XMMATRIX GetModelMatrix() const;
		const DirectX::XMFLOAT4X4* GetModelNDCMat() const;

		const DirectX::XMFLOAT4X4* GetCurrentJointsMatrix(uint32_t* count);
		const DirectX::XMFLOAT4X4* GetInverseJoints(uint32_t* count);

		BasicCam* headCam;
		
	private:
		
		CharacterResource* resourceData;
		Skeleton* skeleton;
		MeshInfo* mesh;
		AnimationManager* animationManager;

		DirectX::XMFLOAT4X4* modelNDCMat;
		DirectX::XMFLOAT4 pos;
		DirectX::XMFLOAT4 direction;
		DirectX::XMFLOAT4 scale;

};


#endif // !MJ_D3D11_CHARACTER_H
 