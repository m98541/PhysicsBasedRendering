#pragma once
#ifndef MJ_D3D11_HALFEDGE_H
#define MJ_D3D11_HALFEDGE_H
#include <DirectXMath.h>
#include <EASTL/vector.h>

namespace HalfEdge
{
	typedef struct HE_VERT_S HE_VERT_T;
	typedef struct HE_EDGE_S HE_EDGE_T;
	typedef struct HE_FACE_S HE_FACE_T;

	typedef struct HE_VERT_S
	{
		DirectX::XMVECTOR pos;
		HE_EDGE_T* edge;
	}HE_VERT_T;

	typedef struct HE_EDGE_S
	{
		HE_VERT_T* originVert;
		HE_VERT_T* vert;
		HE_EDGE_T* pair;
		HE_FACE_T* face;
		HE_EDGE_T* next;

	}HE_EDGE_T;

	typedef struct HE_FACE_S
	{
		HE_EDGE_T* edge;
		DirectX::XMVECTOR norm;

		virtual ~HE_FACE_S() = default;
	}HE_FACE_T;

	typedef struct HE_SET_S
	{
		HE_VERT_T* vertSet;
		unsigned int vertLen;

		HE_FACE_T* faceSet;
		unsigned int faceLen;

		HE_EDGE_T* edgeSet;
		unsigned int edgeLen;

		//하프 엣지 집합 구조 생성 및 확장 시 발생되는 메모리 전부 기록!!
		eastl::vector<void*> memoryRecorder;
	}HE_SET_T;

	typedef struct HE_TRI_EDGE_ADJ_FACE_S
	{
		union
		{
			struct
			{
				HE_FACE_T* face0;
				HE_FACE_T* face1;
				HE_FACE_T* face2;
			};

			struct
			{
				HE_FACE_T* faceArr[3];
			};
		};
		
	}HE_TRI_EDGE_ADJ_FACE_T;

	void CreateHESetFromVertexBuffer(DirectX::XMVECTOR* vertArr, int vertSize,unsigned int* indexArr,int indexSize, HE_SET_T* outHESet);

	HE_TRI_EDGE_ADJ_FACE_T GetFaceAdjFaces(HE_FACE_T* face);

	eastl::vector<HE_EDGE_T*> GetFaceEdges(HE_FACE_T* face);

	eastl::vector<HE_FACE_T*> GetVertAdjFaces(HE_VERT_T* vert);
	
	//추가된 정점과 엣지들은 메모리 기록기에 따로 저장
	void CreateVertEdgesTriFan(HE_EDGE_T** baseEdges, int size, HE_VERT_T* vert, HE_SET_T* outHESet,eastl::vector<HE_FACE_T*>& outNewFaces);

	void DeleteHETriFace(HE_FACE_T* face);


	bool IsBoundaryEdge(HE_EDGE_T* edge);
}
#endif // !MJ_D3D11_HALFEDGE_H

