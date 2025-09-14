#include "MJ_D3D11_HalfEdge.h"

using namespace HalfEdge;
using namespace eastl;

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

	do
	{
		result.push_back(edge->face);
		edge = edge->pair->next;
	} while (edge != vert->edge);

	return result;
}

