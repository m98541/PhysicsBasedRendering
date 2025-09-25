#include "MJ_D3D11_HalfEdge.h"
#include <EASTL/map.h>

using namespace HalfEdge;
using namespace eastl;
using namespace DirectX;


struct EdgeCompare {
	bool operator()(const XMINT2& lhs, const XMINT2& rhs) const {
		// 위와 동일한 비교 로직
		if (lhs.x != rhs.x) return lhs.x < rhs.x;
		return lhs.y < rhs.y;
		
	}


};
bool IsBoundaryEdge(HE_EDGE_T* edge)
{
	return edge->pair == nullptr;
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
	} while (edge != NULL);
	 

	edge = vert->edge;
	if (edge->pair == NULL) return result;

	do//시계 방향 검사
	{
		result.push_back(edge->face);
		edge= edge->pair->next;
		if (edge == vert->edge) return result;
	} while ( edge->pair != NULL);

	return result;
}

void HalfEdge::CreateHESetFromVertexBuffer(DirectX::XMVECTOR* vertArr,int vertSize , unsigned int* indexArr,int indexSize, HE_SET_T* outHESet)
{
	(outHESet)->vertSet = (HE_VERT_T*)malloc(sizeof(HE_VERT_T) * vertSize);
	(outHESet)->vertLen = vertSize;
	if ((outHESet)->vertSet == NULL) return;
	memset((outHESet)->vertSet, NULL, sizeof(HE_VERT_T) * vertSize);

	(outHESet)->faceSet = (HE_FACE_T*)malloc(sizeof(HE_FACE_T) * indexSize / 3);
	(outHESet)->faceLen = indexSize / 3;
	if ((outHESet)->faceSet == NULL) return;

	(outHESet)->edgeSet = (HE_EDGE_T*)malloc(sizeof(HE_EDGE_T) * indexSize);
	(outHESet)->edgeLen = indexSize;
	if ((outHESet)->edgeSet == NULL) return;


	eastl::map<XMINT2, HE_EDGE_T* , EdgeCompare> pairMap;

	
	for (int i = 0; i < indexSize-2; i += 3)
	{

		//origin set
		(outHESet)->edgeSet[i + 0].origin = vertArr[indexArr[i + 0]];
		(outHESet)->edgeSet[i + 1].origin = vertArr[indexArr[i + 1]];
		(outHESet)->edgeSet[i + 2].origin = vertArr[indexArr[i + 2]];
			
		//vertex set
		if ((outHESet)->vertSet[indexArr[i + 0]].edge == NULL)
		{
			(outHESet)->vertSet[indexArr[i + 0]].pos = vertArr[indexArr[i + 0]];
			(outHESet)->vertSet[indexArr[i + 0]].edge = &(outHESet)->edgeSet[i + 0];
		}
		
		if ((outHESet)->vertSet[indexArr[i + 1]].edge == NULL)
		{
			(outHESet)->vertSet[indexArr[i + 1]].pos = vertArr[indexArr[i + 1]];
			(outHESet)->vertSet[indexArr[i + 1]].edge = &(outHESet)->edgeSet[i + 1];
		}
		
		if ((outHESet)->vertSet[indexArr[i + 2]].edge == NULL)
		{
			(outHESet)->vertSet[indexArr[i + 2]].pos = vertArr[indexArr[i + 2]];
			(outHESet)->vertSet[indexArr[i + 2]].edge = &(outHESet)->edgeSet[i + 2];
		}
		
		

		//face set
		(outHESet)->faceSet[i / 3].edge = &(outHESet)->edgeSet[i + 0];

		//edge set
		(outHESet)->edgeSet[i + 0].next = &(outHESet)->edgeSet[i + 1];
		(outHESet)->edgeSet[i + 1].next = &(outHESet)->edgeSet[i + 2];
		(outHESet)->edgeSet[i + 2].next = &(outHESet)->edgeSet[i + 0];

		(outHESet)->edgeSet[i + 0].vert = &(outHESet)->vertSet[indexArr[i + 1]];
		(outHESet)->edgeSet[i + 1].vert = &(outHESet)->vertSet[indexArr[i + 2]];
		(outHESet)->edgeSet[i + 2].vert = &(outHESet)->vertSet[indexArr[i + 0]];

		(outHESet)->edgeSet[i + 0].face = &(outHESet)->faceSet[i / 3];
		(outHESet)->edgeSet[i + 1].face = &(outHESet)->faceSet[i / 3];
		(outHESet)->edgeSet[i + 2].face = &(outHESet)->faceSet[i / 3];

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
				(outHESet)->edgeSet[i + j].pair = NULL;
			}
			else
			{
				pairMap[pairKey]->pair = &(outHESet)->edgeSet[i + j];
				(outHESet)->edgeSet[i + j].pair = pairMap[pairKey];
			}
		}

	}



}


