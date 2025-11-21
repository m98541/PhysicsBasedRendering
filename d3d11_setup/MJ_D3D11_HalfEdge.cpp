#include "MJ_D3D11_HalfEdge.h"
#include "MJ_D3D11_ConvexHull.h"
#include <EASTL/map.h>
#include <EASTL/vector.h> 
using namespace HalfEdge;
using namespace eastl;
using namespace DirectX;


#include <stdio.h>
struct EdgeCompare {
	bool operator()(const XMINT2& lhs, const XMINT2& rhs) const {
		// 위와 동일한 비교 로직
		if (lhs.x != rhs.x) return lhs.x < rhs.x;
		return lhs.y < rhs.y;
		
	}
};


void HalfEdge::CreateVertEdgesTriFan(HE_EDGE_T** baseEdges, int size, HE_VERT_T* vert, HE_SET_T* outHESet, eastl::vector<HE_FACE_T*>& outNewFaces)
{

	if (baseEdges == nullptr) return;



	for (int i = 0; i < size && baseEdges[i] != nullptr; i++)
	{

		HE_FACE_T* face = (HE_FACE_T*)malloc(sizeof(ConvexHullFace_T));

		if (face == nullptr) return;

		outHESet->memoryRecorder.push_back(face);
		ConvexHullFace_T* pCastingFace = static_cast<ConvexHullFace_T*>(face);
		new (pCastingFace) ConvexHullFace_T();

		HE_EDGE_T* baseEdgePair = (HE_EDGE_T*)malloc(sizeof(HE_EDGE_T));
		if (baseEdgePair == nullptr) return;
		outHESet->memoryRecorder.push_back(baseEdgePair);



		HE_EDGE_T* lineEdge = (HE_EDGE_T*)malloc(sizeof(HE_EDGE_T));

		if (lineEdge == nullptr) return;

		outHESet->memoryRecorder.push_back(lineEdge);



		HE_EDGE_T* lineEdgePair = (HE_EDGE_T*)malloc(sizeof(HE_EDGE_T));

		if (lineEdgePair == nullptr) return;

		outHESet->memoryRecorder.push_back(lineEdgePair);




		baseEdgePair->face = pCastingFace;


		baseEdges[i]->pair = baseEdgePair;

		baseEdgePair->pair = baseEdges[i];



		baseEdgePair->vert = baseEdges[i]->originVert;
		baseEdgePair->originVert = baseEdges[i]->vert;
		baseEdgePair->originVert->edge = baseEdgePair;

		lineEdge->face = pCastingFace;

		lineEdge->pair = lineEdgePair;

		lineEdgePair->pair = lineEdge;

		lineEdge->vert = vert;

		lineEdge->originVert = baseEdgePair->vert;


		lineEdgePair->vert = lineEdge->originVert;

		lineEdgePair->originVert = lineEdge->vert;


		if (i == 0)
		{

			vert->edge = lineEdgePair;

		}

		baseEdgePair->next = lineEdge;

		face->edge = baseEdgePair;
		XMVECTOR v1 = baseEdgePair->vert->pos - baseEdgePair->originVert->pos;
		XMVECTOR v2 = lineEdge->vert->pos - lineEdge->originVert->pos;
		face->norm = XMVector3Normalize(XMVector3Cross(v1, v2));


	}



	for (int i = 0; i < size && baseEdges[i] != nullptr; i++)
	{

		HE_EDGE_T* baseEdgePair = baseEdges[i]->pair;
		HE_EDGE_T* lineEdge = baseEdgePair->next;
		HE_EDGE_T* lineEdgePair = lineEdge->pair;

		lineEdgePair->next = lineEdgePair->vert->edge;
		lineEdgePair->next->face = lineEdgePair->vert->edge->face;

		lineEdgePair->next->next->next = lineEdgePair;
		lineEdgePair->next->next->next->face = lineEdgePair->vert->edge->face;
		lineEdgePair->face = lineEdgePair->next->face;

		outNewFaces.push_back(lineEdgePair->face);

	}

}



void HalfEdge::DeleteHETriFace(HE_FACE_T* face)
{
	
	HE_EDGE_T* edge0 = face->edge;
	HE_EDGE_T* edge1 = edge0->next;
	HE_EDGE_T* edge2 = edge1->next;

	
	HE_VERT_T* vert0 = edge0->originVert;
	HE_VERT_T* vert1 = edge1->originVert;
	HE_VERT_T* vert2 = edge2->originVert;

	if (edge0->pair != nullptr) edge0->pair->pair = nullptr;
	if (edge1->pair != nullptr) edge1->pair->pair = nullptr;
	if (edge2->pair != nullptr) edge2->pair->pair = nullptr;

	if (vert0->edge == edge0)
	{
		if (edge0->pair != nullptr)
		{
			vert0->edge = edge0->pair->next;
		}
		else if (edge2->pair != nullptr)
		{
			vert0->edge = edge2->pair;
		}
		else
		{
			vert0->edge = nullptr;
		}
	}


	if (vert1->edge == edge1)
	{
		if (edge1->pair != nullptr)
		{
			vert1->edge = edge1->pair->next;
		}
		else if (edge0->pair != nullptr)
		{
			vert1->edge = edge0->pair;
		}
		else
		{
			vert1->edge = nullptr;
		}

	}


	if (vert2->edge == edge2)
	{
		if (edge2->pair != nullptr)
		{
			vert2->edge = edge2->pair->next;
		}
		else if (edge1->pair != nullptr)
		{
			vert2->edge = edge1->pair;
		}
		else
		{
			vert2->edge = nullptr;
		}

	}


	edge0->pair = nullptr;
	edge1->pair = nullptr;
	edge2->pair = nullptr;
	
}

bool HalfEdge::IsBoundaryEdge(HE_EDGE_T* edge)
{
	return edge->pair == nullptr;
}

HE_TRI_EDGE_ADJ_FACE_T HalfEdge::GetFaceAdjFaces(HE_FACE_T* face)
{
	HE_TRI_EDGE_ADJ_FACE_T adjFace;
	if (face->edge->pair != nullptr)
	{
		adjFace.face0 = face->edge->pair->face;
	}		
	else
	{
		adjFace.face0 = nullptr;
	}

	if (face->edge->next->pair != nullptr)
	{
		adjFace.face1 = face->edge->next->pair->face;
	}
	else
	{
		adjFace.face1 = nullptr;
	}
		

	if (face->edge->next->next->pair != nullptr)
	{
		adjFace.face2 = face->edge->next->next->pair->face;
	}
	else
	{
		adjFace.face2 = nullptr;
	}

	return adjFace;
}

vector<HE_EDGE_T*> HalfEdge::GetFaceEdges(HE_FACE_T* face)
{
	vector<HE_EDGE_T*> result;

	HE_EDGE_T* edge = face->edge;

	do
	{
		result.push_back(edge);
		edge = edge->next;
	} while (edge != face->edge);

	return result;
}

vector<HE_FACE_T*> HalfEdge::GetVertAdjFaces(HE_VERT_T* vert)
{
	vector<HE_FACE_T*> result;
	
	
	HE_EDGE_T* edge = vert->edge;

	do//반시계 방향 검사
	{
		result.push_back(edge->face);
		edge = edge->next->next->pair;
		if (edge == vert->edge) return result;
	} while (edge != nullptr);
	 

	edge = vert->edge;
	if (edge->pair == nullptr) return result;

	do//시계 방향 검사
	{
		result.push_back(edge->face);
		edge= edge->pair->next;
		if (edge == vert->edge) return result;
	} while ( edge->pair != nullptr);

	return result;
}

void HalfEdge::CreateHESetFromVertexBuffer(DirectX::XMVECTOR* vertArr,int vertSize , unsigned int* indexArr,int indexSize, HE_SET_T* outHESet)
{
	(outHESet)->vertSet = (HE_VERT_T*)malloc(sizeof(HE_VERT_T) * vertSize);
	(outHESet)->vertLen = vertSize;
	if ((outHESet)->vertSet == nullptr) return;
	outHESet->memoryRecorder.push_back((outHESet)->vertSet);

	memset((outHESet)->vertSet, NULL, sizeof(HE_VERT_T) * vertSize);

	(outHESet)->faceSet = (HE_FACE_T*)malloc(sizeof(ConvexHullFace_T) * indexSize / 3);
	(outHESet)->faceLen = indexSize / 3;
	if ((outHESet)->faceSet == nullptr) return;
	outHESet->memoryRecorder.push_back((outHESet)->faceSet);
	
	ConvexHullFace_T* pCastingFaceSet = static_cast<ConvexHullFace_T*>((outHESet)->faceSet);
	pCastingFaceSet->curFarthestPoint.point = nullptr;
	pCastingFaceSet->curFarthestPoint.len = 0.0;
	pCastingFaceSet->curFarthestPoint.face = pCastingFaceSet;


	for (int i = 0; i < indexSize / 3; i++)
	{
	
		new (&pCastingFaceSet[i]) ConvexHullFace_T();

	}

	(outHESet)->edgeSet = (HE_EDGE_T*)malloc(sizeof(HE_EDGE_T) * indexSize);
	(outHESet)->edgeLen = indexSize;
	if ((outHESet)->edgeSet == nullptr) return;
	outHESet->memoryRecorder.push_back((outHESet)->edgeSet);


	eastl::map<XMINT2, HE_EDGE_T* , EdgeCompare> pairMap;

	
	for (int i = 0; i < indexSize-2; i += 3)
	{

		
			
		//vertex set
		if ((outHESet)->vertSet[indexArr[i + 0]].edge == nullptr)
		{
			(outHESet)->vertSet[indexArr[i + 0]].pos = vertArr[indexArr[i + 0]];
			(outHESet)->vertSet[indexArr[i + 0]].edge = &(outHESet)->edgeSet[i + 0];
		}
		
		if ((outHESet)->vertSet[indexArr[i + 1]].edge == nullptr)
		{
			(outHESet)->vertSet[indexArr[i + 1]].pos = vertArr[indexArr[i + 1]];
			(outHESet)->vertSet[indexArr[i + 1]].edge = &(outHESet)->edgeSet[i + 1];
		}
		
		if ((outHESet)->vertSet[indexArr[i + 2]].edge == nullptr)
		{
			(outHESet)->vertSet[indexArr[i + 2]].pos = vertArr[indexArr[i + 2]];
			(outHESet)->vertSet[indexArr[i + 2]].edge = &(outHESet)->edgeSet[i + 2];
		}
		
		

		//face set
		pCastingFaceSet[i / 3].edge = &(outHESet)->edgeSet[i + 0];
	
		XMVECTOR v0 = vertArr[indexArr[i + 0]];
		XMVECTOR v1 = vertArr[indexArr[i + 1]];
		XMVECTOR v2 = vertArr[indexArr[i + 2]];

		pCastingFaceSet[i / 3].norm = XMVector3Normalize(XMVector3Cross(
			(v1 - v0),
			(v2 - v0)
		));
	
		//edge set
		(outHESet)->edgeSet[i + 0].next = &(outHESet)->edgeSet[i + 1];
		(outHESet)->edgeSet[i + 1].next = &(outHESet)->edgeSet[i + 2];
		(outHESet)->edgeSet[i + 2].next = &(outHESet)->edgeSet[i + 0];

		(outHESet)->edgeSet[i + 0].vert = &(outHESet)->vertSet[indexArr[i + 1]];
		(outHESet)->edgeSet[i + 1].vert = &(outHESet)->vertSet[indexArr[i + 2]];
		(outHESet)->edgeSet[i + 2].vert = &(outHESet)->vertSet[indexArr[i + 0]];
		
		(outHESet)->edgeSet[i + 0].originVert = &(outHESet)->vertSet[indexArr[i + 0]]; 
		(outHESet)->edgeSet[i + 1].originVert = &(outHESet)->vertSet[indexArr[i + 1]]; 
		(outHESet)->edgeSet[i + 2].originVert = &(outHESet)->vertSet[indexArr[i + 2]]; 

		(outHESet)->edgeSet[i + 0].face = &pCastingFaceSet[i / 3];
		(outHESet)->edgeSet[i + 1].face = &pCastingFaceSet[i / 3];
		(outHESet)->edgeSet[i + 2].face = &pCastingFaceSet[i / 3];

	
		for (int j = 0; j < 3; j++)
		{
			XMINT2 pairKey = { (int)indexArr[i + j] , (int)indexArr[i + (j+1)%3]};
			if (pairKey.x < pairKey.y)
			{
				int temp = pairKey.x;
				pairKey.x = pairKey.y;
				pairKey.y = temp;
			}
			
			if(pairMap.count(pairKey) == 0)
			{
				pairMap[pairKey] = &(outHESet)->edgeSet[i + j];
			
				(outHESet)->edgeSet[i + j].pair = nullptr;
			}
			else
			{
				pairMap[pairKey]->pair = &(outHESet)->edgeSet[i + j];
				(outHESet)->edgeSet[i + j].pair = pairMap[pairKey];
				
			}
		}

	}



}


