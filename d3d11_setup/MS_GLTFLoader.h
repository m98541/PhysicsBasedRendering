#ifndef MS_GLTFLOADER_H
#define MS_GLTFLOADER_H
//Based on Microsoft's GLTF_SDK
#include <GLTFSDK/Deserialize.h>
#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLTFResourceReader.h>
#include <GLTFSDK/IStreamReader.h>
#include <fstream>
#include <string>
#include <memory>
#include "MJ_D3D11_Skeleton.h"


class MSGLTFLoader {
	public:
		MSGLTFLoader();
		~MSGLTFLoader();
		MSGLTFLoader(const std::string& path);
		bool LoadModel(const std::string& path);

		//내부 데이터로 변환 하여 스켈레톤 데이터 통일
		void GetSkeletonResource(SkeletonResource_T& outData);
	private:
		Microsoft::glTF::Document document;
		std::unique_ptr<Microsoft::glTF::GLTFResourceReader> resourceReader;

		
};


#endif // !MS_GLTFLOADER_H
