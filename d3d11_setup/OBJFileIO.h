#ifndef OBJFILEIO_H
#define OBJFILEIO_H

#define LINE_TYPE_NOP	-1
#define LINE_TYPE_V		0 // 0000
#define LINE_TYPE_VT	1 // 0001
#define LINE_TYPE_VN	2 // 0010
#define LINE_TYPE_F		3 
#define LINE_TYPE_O		7
#define LINE_TYPE_G		6
#define LINE_TYPE_MTL_NAME 4
#define LINE_TYPE_MTL_USE 5

#define MTL_LINE_TYPE_NEWMTL 0
#define MTL_LINE_TYPE_KA 1
#define MTL_LINE_TYPE_KD 2
#define MTL_LINE_TYPE_KS 3
#define MTL_LINE_TYPE_NS 4
#define MTL_LINE_TYPE_D  5 // 불투명도: 0.0 투명 1.0 불투명
#define MTL_LINE_TYPE_TR 6 // 투명도(d 와 반대) TR 발견시 1-TR 로하여금 d 로 변환
#define MTL_LINE_TYPE_ILLUM 7
#define MTL_LINE_TYPE_MAP_KD 8 // diffuse 텍스처 이미지 즉 기본 텍스처
#define MTL_LINE_TYPE_BUMP 9 // 범프맵 강도 설정
#define MTL_LINE_TYPE_MAP_BUMP 10 //범프맵 이미지 경로 지정
#define MTL_LINE_TYPE_MAP_D 11
#define MTL_LINE_TYPE_NI 12
#define MTL_LINE_TYPE_KE 13
#define MTL_LINE_TYPE_MAP_KS 14
#define MTL_LINE_TYPE_TD 15

#define FACE_FORMAT_V 0 // 0000
#define FACE_FORMAT_V_T 1 // 0001
#define FACE_FORMAT_V_N 2 // 0010
#define FACE_FORMAT_V_N_T 3 // 0011

#define PRIMITIVE_TRIANGLE 3

#define CHAR_NAME_MAX 256

#define ERROR -1;


typedef struct MTL_ELEMENT
{
	unsigned int mtlKeyWordEnable[15];//0 이면 비활성화 상태임
	char materialName[CHAR_NAME_MAX];
	float ambient[4];
	float diffuse[4];
	float specular[4];
	float shine[4];
	float alphaValue;
	float bump;
	float ni;
	float ke[3];
	unsigned int lightModel;
	//사용 안할시 null 처리
	char textureMapImageName[CHAR_NAME_MAX];
	char bumpMapImageName[CHAR_NAME_MAX];
	char specularMapImageName[CHAR_NAME_MAX];
	char alphaMapImageName[CHAR_NAME_MAX];

	unsigned int textureIdx;
}MLT_ELEMENT_T;



typedef struct OBJFILE_DESC
{
	const char* fileName;
	char mtlFileName[CHAR_NAME_MAX];

	unsigned int faceFormat;

	union
	{
		struct Len{
			unsigned int vert;
			unsigned int tex;
			unsigned int normal;
			unsigned int face;
			unsigned int mtl;
			unsigned int useMtl;
			unsigned int group;
		}len;

		unsigned int lenArr[7];
	};

	union 
	{
		struct ElementCount {
			unsigned int vert;
			unsigned int tex;
			unsigned int normal;
			unsigned int face;
		}elementCount;

		unsigned int elementCountArr[4];

	};
	

}OBJFILE_DESC_T;



typedef struct TRI_POLYGON
{
	unsigned int polygonOffset;
	unsigned int vertexCount;
}TRI_POLYGON_T;


typedef struct TRI_POLYGON_GROUP
{
	unsigned int groupOffset;//group 시작 주소 
	unsigned int vertexCount;//시작 주소 부터 어디까지 렌더링 할지 정보 저장
}TRI_POLYGON_GROUP_T;


typedef struct OBJFILE_BUFFER
{
	// size : buffer 의 byte 단위 전체 크기
	// len : buffer 단위 요소 수 size / 단위 요소 크기
	unsigned int faceLen;

	MTL_ELEMENT* mtlBuffer;
	unsigned int mtlBufferSize;
	unsigned int mtlBufferLen;

	
	char (*textureImageNameArr)[CHAR_NAME_MAX];
	unsigned int textureImageNameLen;

	float* vertBuffer;
	unsigned int  vertBufferSize;
	unsigned int  vertBufferLen;

	float* texBuffer;// 텍스처 좌표에 대한 버퍼(텍스처 내에서 어떤 부분인지) 
	unsigned int  texBufferSize;
	unsigned int  texBufferLen;

	float* normBuffer;
	unsigned int  normBufferSize;
	unsigned int  normBufferLen;
	
	float* objectBuffer;
	unsigned int  objectBufferLineSize;
	unsigned int  objectBufferSize;
	unsigned int  objectBufferLen;

	unsigned int* materialIdxBuffer;// 각정점에 어떤 material 속성을 사용할지 결정 버퍼
	//objectBuffer 와 1대1 맵핑관계


	TRI_POLYGON_GROUP_T* groupSet;
	unsigned int groupSetLen;

	TRI_POLYGON_T* polygonSet;
	unsigned int polygonSetLen;

}OBJFILE_BUFFER_T;


bool ParseOBJFile(OBJFILE_DESC* ObjectDesc, const char* FileName);

bool ReadOBJFile(OBJFILE_DESC_T* OBJFileDESC, OBJFILE_BUFFER_T** OBJFileBuffer);

void ReleaseOBJBuffer(OBJFILE_BUFFER_T** OBJFileBuffer);


#endif // !OBJFILEIO_H
