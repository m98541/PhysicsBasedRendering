#ifndef MJ_D3D11_CHARACTER_H
#define MJ_D3D11_CHARACTER_H
#define NOMINMAX
#include "MJ_D3D11_CharacterResource.h"
#include "MJ_D3D11_Skeleton.h"
#include "MJ_D3D11_Mesh.h"
#include "MS_GLTFLoader.h"

// 캐릭터 데이터에 대한 소유 및 상태 변화 를 책임 렌더러에 필요데이터 전달해줌 , 전달 데이터에 대한 책임을 짐

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
		void update();

		const MeshInfo* GetMeshInfo() const;
		const Skeleton* GetSkeleton() const;
		const TextureResourec_T* GetTextureResource() const;
	private:
		CharacterResource* resourceData;
		Skeleton* skeleton;
		MeshInfo* mesh;
	
};


#endif // !MJ_D3D11_CHARACTER_H
 