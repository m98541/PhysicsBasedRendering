#ifndef MS_GLTFLOADER_H
#define MS_GLTFLOADER_H
//Based on Microsoft's GLTF_SDK
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/IStreamReader.h>
#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/string.h>
#include <EASTL/queue.h>
#include <DirectXMath.h>
#include <fstream>
#include <string>
#include <memory>
#include <DirectXTex.h>
#include "MJ_D3D11_Character.h"

//클래스 이름 변경 필요 GLTF LOADER 아님 GLTF CHRACTER LOADER 로 변경 필요
class MSGLTFLoader {
	public:
		MSGLTFLoader();
		~MSGLTFLoader();
		MSGLTFLoader(const std::string& path);
		bool LoadModel(const std::string& path);

		// 네이밍 변경 필요 Get -> Load 
		//내부 데이터로 변환 하여 스켈레톤 데이터 통일
		void GetCharacterResource(CharacterResource* resourceData);

		//private 로 이동 예정 외부에서는 GetCharacterResource만 보이도록 할 것임
		void GetSkeletonResource(SkeletonResource_T& outData);
		void GetMeshResource(MeshResource_T& outDatasrcSkin , eastl::map<eastl::string, int>& textureIdMap);
		void GetTextureResource(TextureResourec_T& outTexData , eastl::map<eastl::string , int>& textureIdMap);
		void GetAnimationResource(AnimationResource_T& outData , size_t jointsCount);

	private:
		void JointsDataSort(Joint_T* data, size_t size);
		void PosDataNormalization(CharacterResource* resourceData);
		Microsoft::glTF::Document document;
		std::unique_ptr<Microsoft::glTF::GLTFResourceReader> resourceReader;

		//str id 와 jointResource 의 index 간 번역을 위한 집합 테이블
		eastl::map<eastl::string , int> JointIdToIndexTable;
		eastl::map<eastl::string, int> nodeIdToIndexTable;
};


#endif // !MS_GLTFLOADER_H
