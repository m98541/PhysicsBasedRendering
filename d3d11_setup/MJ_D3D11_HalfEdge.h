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
		DirectX::XMVECTOR origin;
		HE_VERT_T* vert;
		HE_EDGE_T* pair;
		HE_FACE_T* face;
		HE_EDGE_T* next;

	}HE_EDGE_T;

	typedef struct HE_FACE_S
	{
		HE_EDGE_T* edge;
	}HE_FACE_T;

	typedef struct HE_SET_S
	{
		HE_VERT_T* vertSet;
		unsigned int vertLen;

		HE_FACE_T* faceSet;
		unsigned int faceLen;

		HE_EDGE_T* edgeSet;
		unsigned int edgeLen;
	}HE_SET_T;

	void CreateHESetFromVertexBuffer(DirectX::XMVECTOR* vertArr, int vertSize,unsigned int* indexArr,int indexSize, HE_SET_T* outHESet);

	eastl::vector<HE_EDGE_T*> GetFaceEdges(HE_FACE_T* face);

	eastl::vector<HE_FACE_T*> GetVertAdjFaces(HE_VERT_T* vert);

	bool IsBoundaryEdge(HE_EDGE_T* edge);
}
#endif // !MJ_D3D11_HALFEDGE_H

