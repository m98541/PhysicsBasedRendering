#include "MJ_D3D11_ConvexHull.h"
#include <EASTL/vector.h>
#include <EASTL/set.h>
#include <EASTL/sort.h>
#include <EASTL/map.h>
#include <float.h>
#include <stdio.h>
using namespace DirectX;
using namespace eastl;
using namespace HalfEdge;



inline ConvexHullFace_T* ToConvexHullFace(HalfEdge::HE_FACE_T* face)
{
	return static_cast<ConvexHullFace_T*>(face);
}
inline HalfEdge::HE_FACE_T* ToHEFace(ConvexHullFace_T* convexHullFace)
{
	return convexHullFace;
}

struct VertexCompare {
	bool operator()(const VERTEX_T& lhs, const VERTEX_T& rhs) const {
		// 위와 동일한 비교 로직
		if (lhs.pos.x != rhs.pos.x) return lhs.pos.x < rhs.pos.x;
		if (lhs.pos.y != rhs.pos.y) return lhs.pos.y < rhs.pos.y;
		if (lhs.pos.z != rhs.pos.z) return lhs.pos.z < rhs.pos.z;
		if (lhs.pos.w != rhs.pos.w) return lhs.pos.w < rhs.pos.w;
		if (lhs.tex.x != rhs.tex.x) return lhs.tex.x < rhs.tex.x;
		if (lhs.tex.y != rhs.tex.y) return lhs.tex.y < rhs.tex.y;
		if (lhs.norm.x != rhs.norm.x) return lhs.norm.x < rhs.norm.x;
		if (lhs.norm.y != rhs.norm.y) return lhs.norm.y < rhs.norm.y;
		if (lhs.norm.z != rhs.norm.z) return lhs.norm.z < rhs.norm.z;
		if (lhs.norm.w != rhs.norm.w) return lhs.norm.w < rhs.norm.w;
		return lhs.textureIdx < rhs.textureIdx;
	}


};

ConvexHull::ConvexHull() : Convex(CONVEXHULL)
{
	this->simplexSet = nullptr;
	this->initSimplex = nullptr;

}
ConvexHull::ConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size): Convex(CONVEXHULL)
{
    //초기 심플랙스 구성
	CreateConvexHull(inVertexArray, size);
}

ConvexHull::~ConvexHull()
{
	for (auto const& mem : this->simplexSet->memoryRecorder) {
		free(mem);
	}
	delete initSimplex;
}

/*
// 삭제된 면들과 외각선을 집합과 map에 담아줌
void RecursiveDeleteFaceSearch(ConvexHullFace_T* face, HE_VERT_T* extensionPoint ,vector<ConvexHullFace_T*>* searchFaces, map<HE_EDGE_T*, HE_EDGE_T*>* searchEdgesMap )
{
	if (face == nullptr) return;

	printf("search face\n");
	printf("face : %f %f %f %f ->%f %f %f %f ->%f %f %f %f  face addr : %x\n"
		, face->edge->originVert->pos.m128_f32[0]
		, face->edge->originVert->pos.m128_f32[1]
		, face->edge->originVert->pos.m128_f32[2]
		, face->edge->originVert->pos.m128_f32[3]

		, face->edge->next->originVert->pos.m128_f32[0]
		, face->edge->next->originVert->pos.m128_f32[1]
		, face->edge->next->originVert->pos.m128_f32[2]
		, face->edge->next->originVert->pos.m128_f32[3]

		, face->edge->next->next->originVert->pos.m128_f32[0]
		, face->edge->next->next->originVert->pos.m128_f32[1]
		, face->edge->next->next->originVert->pos.m128_f32[2]
		, face->edge->next->next->originVert->pos.m128_f32[3]
		, face);


	for (size_t i = 0; i < searchFaces->size(); ++i)
	{
		if ((*searchFaces)[i] == face)
		{
			// 이미 처리중인 면이므로, 재귀를 중단합니다.
			return;
		}
	}

	if (XMVector3Dot(face->norm, (extensionPoint->pos)).m128_f32[0] > 0)// 점에서 보이는 면 확인
	{
		HE_EDGE_T* edge = face->edge;
		auto it = eastl::find(searchFaces->begin() , searchFaces->end() , face);
		if (it == searchFaces->end())
		{
			searchFaces->push_back(face);
		}
		
		do
		{
		
			if (searchEdgesMap->count(edge->pair) == 1)
			{
				searchEdgesMap->erase(edge->pair);
			}
			else
			{
				 
				//해당 엣지 반대 페이스가 존재 함으로 현재 엣지 저장후 해당 페이스 탐색
				(*searchEdgesMap)[edge] = edge;
				
				if (edge->pair == nullptr)
				{
					printf("edge null ! : %f %f %f %f <- %f %f %f %f \nface : %f %f %f %f->%f %f %f %f->%f %f %f %f  face addr : %x" 
						, edge->vert->pos.m128_f32[0]
						, edge->vert->pos.m128_f32[1]
						, edge->vert->pos.m128_f32[2]
						, edge->vert->pos.m128_f32[3]
						, edge->originVert->pos.m128_f32[0]
						, edge->originVert->pos.m128_f32[1]
						, edge->originVert->pos.m128_f32[2]
						, edge->originVert->pos.m128_f32[3]
						, edge->originVert->pos.m128_f32[0]
						, edge->originVert->pos.m128_f32[1]
						, edge->originVert->pos.m128_f32[2]
						, edge->originVert->pos.m128_f32[3]
						, edge->next->originVert->pos.m128_f32[0]
						, edge->next->originVert->pos.m128_f32[1]
						, edge->next->originVert->pos.m128_f32[2]
						, edge->next->originVert->pos.m128_f32[3]
						, edge->next->next->originVert->pos.m128_f32[0]
						, edge->next->next->originVert->pos.m128_f32[1]
						, edge->next->next->originVert->pos.m128_f32[2]
						, edge->next->next->originVert->pos.m128_f32[3]
						, edge->face);
					
					assert(edge->pair == nullptr && " find zombie edge!");
				
				}
				else
				{

					RecursiveDeleteFaceSearch(ToConvexHullFace(edge->pair->face), extensionPoint, searchFaces, searchEdgesMap);
				}

				
			}
			edge = edge->next;

		} while (edge != face->edge);
		
		
	}
}
*/

void DeleteFaceSearch(vector<ConvexHullFace_T*> activeFace ,ConvexHullFace_T* face, HE_VERT_T* extensionPoint, vector<ConvexHullFace_T*>* searchFaces, map<HE_EDGE_T*, HE_EDGE_T*>* searchEdgesMap)
{
	//pass-1 search face
	if (searchFaces == nullptr || searchEdgesMap == nullptr || extensionPoint == nullptr)
	{
		return; // 혹은 assert
	}
	for (auto const& faceDelete : activeFace)
	{
		XMVECTOR p0 = faceDelete->edge->originVert->pos;
		if (XMVector3Dot(faceDelete->norm, extensionPoint->pos - p0).m128_f32[0] > 0) 
		{
			searchFaces->push_back(faceDelete);
		}
	}

	//pass-2
	for (auto const& faceDelete : *searchFaces)
	{
		HE_EDGE_T* edge0 = faceDelete->edge->pair;
		HE_EDGE_T* edge1 = faceDelete->edge->next->pair;
		HE_EDGE_T* edge2 = faceDelete->edge->next->next->pair;
		
		if (edge0 == nullptr)
		{

		}
		auto it = find(searchFaces->begin(), searchFaces->end(), edge0->face);
		if (it == searchFaces->end())
		{
			(*searchEdgesMap)[edge0] = edge0;
		}

		it = find(searchFaces->begin(), searchFaces->end(), edge1->face);
		if (it == searchFaces->end())
		{
			(*searchEdgesMap)[edge1] = edge1;
		}

		it =find(searchFaces->begin(), searchFaces->end(), edge2->face);
		if (it == searchFaces->end())
		{
			(*searchEdgesMap)[edge2] = edge2;
		}
	}
}

void ConvexHull::ExtensionHull(ConvexHullFace_T* startFace ,XMVECTOR* extensionPoint)
{
	printf("start face : %f %f %f %f ->%f %f %f %f ->%f %f %f %f  face addr : %x\n"
		, startFace->edge->originVert->pos.m128_f32[0]
		, startFace->edge->originVert->pos.m128_f32[1]
		, startFace->edge->originVert->pos.m128_f32[2]
		, startFace->edge->originVert->pos.m128_f32[3]

		, startFace->edge->next->originVert->pos.m128_f32[0]
		, startFace->edge->next->originVert->pos.m128_f32[1]
		, startFace->edge->next->originVert->pos.m128_f32[2]
		, startFace->edge->next->originVert->pos.m128_f32[3]

		, startFace->edge->next->next->originVert->pos.m128_f32[0]
		, startFace->edge->next->next->originVert->pos.m128_f32[1]
		, startFace->edge->next->next->originVert->pos.m128_f32[2]
		, startFace->edge->next->next->originVert->pos.m128_f32[3]
		, startFace);

	printf("현재 활성 면 \n");
	for (auto const& face : this->activeFaces)
	{
		printf("face : %f %f %f %f ->%f %f %f %f ->%f %f %f %f  face addr : %x\n"
			, face->edge->originVert->pos.m128_f32[0]
			, face->edge->originVert->pos.m128_f32[1]
			, face->edge->originVert->pos.m128_f32[2]
			, face->edge->originVert->pos.m128_f32[3]

			, face->edge->next->originVert->pos.m128_f32[0]
			, face->edge->next->originVert->pos.m128_f32[1]
			, face->edge->next->originVert->pos.m128_f32[2]
			, face->edge->next->originVert->pos.m128_f32[3]

			, face->edge->next->next->originVert->pos.m128_f32[0]
			, face->edge->next->next->originVert->pos.m128_f32[1]
			, face->edge->next->next->originVert->pos.m128_f32[2]
			, face->edge->next->next->originVert->pos.m128_f32[3]
			, face);
	}
	printf("\n");

	vector<ConvexHullFace_T*> deleteFaces;
	vector<HE_EDGE_T*> baseEdgeSet;
	vector<DirectX::XMVECTOR*> newPointSet;
	map<HE_EDGE_T*, HE_EDGE_T*> baseEdgeMap;

	HE_VERT_T* newExtexsionVert = (HE_VERT_T*)malloc(sizeof(HE_VERT_T));
	if (newExtexsionVert == nullptr) return; 
	newExtexsionVert->pos = *(extensionPoint); 
	newExtexsionVert->edge = nullptr; 

	simplexSet->memoryRecorder.push_back(newExtexsionVert);
	
	DeleteFaceSearch(this->activeFaces , startFace, newExtexsionVert, &deleteFaces, &baseEdgeMap);
	
	printf("외각 경계면\n");
	for (auto const& baseEdge : baseEdgeMap) {
		baseEdgeSet.push_back(baseEdge.second); 
		printf("edge vert %f %f %f %f <- origin vert %f %f %f %f, "
			, baseEdge.second->vert->pos.m128_f32[0]
			, baseEdge.second->vert->pos.m128_f32[1]
			, baseEdge.second->vert->pos.m128_f32[2]
			, baseEdge.second->vert->pos.m128_f32[3]
			, baseEdge.second->originVert->pos.m128_f32[0]
			, baseEdge.second->originVert->pos.m128_f32[1]
			, baseEdge.second->originVert->pos.m128_f32[2]
			, baseEdge.second->originVert->pos.m128_f32[3]);

		if (baseEdge.second->pair != nullptr)
		{
			printf("edge pair vert %f %f %f %f <- origin vert %f %f %f %f,\n"
				, baseEdge.second->pair->vert->pos.m128_f32[0]
				, baseEdge.second->pair->vert->pos.m128_f32[1]
				, baseEdge.second->pair->vert->pos.m128_f32[2]
				, baseEdge.second->pair->vert->pos.m128_f32[3]
				, baseEdge.second->pair->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->originVert->pos.m128_f32[3]);
			printf("next edge pair vert %f %f %f %f <- origin vert %f %f %f %f, \n"
				, baseEdge.second->pair->next->vert->pos.m128_f32[0]
				, baseEdge.second->pair->next->vert->pos.m128_f32[1]
				, baseEdge.second->pair->next->vert->pos.m128_f32[2]
				, baseEdge.second->pair->next->vert->pos.m128_f32[3]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[3]);

		}
		else
		{
			printf("pair : null\n");
		}
		
	}



	eastl_size_t newPointSetSize = 0;
	for (int i = 0; i < deleteFaces.size(); i++)
	{
		newPointSetSize += deleteFaces[i]->pointSet.size();
	}
	newPointSet.reserve(newPointSetSize);
	set<ConvexHullFace_T*> deleteFaceSet(deleteFaces.begin(), deleteFaces.end());// 벡터구조로 자료 모은 뒤 집합(레드블랙트리) 형태 변환
	printf("삭제면 \n");
	for (int i = activeFaces.size() - 1; i >= 0; i--)
	{
		if (deleteFaceSet.count(activeFaces[i]))
		{
			printf("face : %f %f %f %f ->%f %f %f %f ->%f %f %f %f  face addr : %x\n"
				, activeFaces[i]->edge->originVert->pos.m128_f32[0]
				, activeFaces[i]->edge->originVert->pos.m128_f32[1]
				, activeFaces[i]->edge->originVert->pos.m128_f32[2]
				, activeFaces[i]->edge->originVert->pos.m128_f32[3]

				, activeFaces[i]->edge->next->originVert->pos.m128_f32[0]
				, activeFaces[i]->edge->next->originVert->pos.m128_f32[1]
				, activeFaces[i]->edge->next->originVert->pos.m128_f32[2]
				, activeFaces[i]->edge->next->originVert->pos.m128_f32[3]

				, activeFaces[i]->edge->next->next->originVert->pos.m128_f32[0]
				, activeFaces[i]->edge->next->next->originVert->pos.m128_f32[1]
				, activeFaces[i]->edge->next->next->originVert->pos.m128_f32[2]
				, activeFaces[i]->edge->next->next->originVert->pos.m128_f32[3]
				, activeFaces[i]);
			activeFaces[i] = activeFaces.back();
			
			activeFaces.pop_back();
		}
	}
	
	for (int i = 0; i < deleteFaces.size(); i++)
	{
		newPointSet.insert(newPointSet.end() , deleteFaces[i]->pointSet.begin() , deleteFaces[i]->pointSet.end());
		DeleteHETriFace(deleteFaces[i]);
	}
	printf("삭제후\n");
	for (auto const& baseEdge : baseEdgeMap) {
		printf("edge vert %f %f %f %f <- origin vert %f %f %f %f, "
			, baseEdge.second->vert->pos.m128_f32[0]
			, baseEdge.second->vert->pos.m128_f32[1]
			, baseEdge.second->vert->pos.m128_f32[2]
			, baseEdge.second->vert->pos.m128_f32[3]
			, baseEdge.second->originVert->pos.m128_f32[0]
			, baseEdge.second->originVert->pos.m128_f32[1]
			, baseEdge.second->originVert->pos.m128_f32[2]
			, baseEdge.second->originVert->pos.m128_f32[3]);

		if (baseEdge.second->pair != nullptr)
		{
			printf("edge pair vert %f %f %f %f <- origin vert %f %f %f %f,\n"
				, baseEdge.second->pair->vert->pos.m128_f32[0]
				, baseEdge.second->pair->vert->pos.m128_f32[1]
				, baseEdge.second->pair->vert->pos.m128_f32[2]
				, baseEdge.second->pair->vert->pos.m128_f32[3]
				, baseEdge.second->pair->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->originVert->pos.m128_f32[3]);
			printf("next edge pair vert %f %f %f %f <- origin vert %f %f %f %f, \n"
				, baseEdge.second->pair->next->vert->pos.m128_f32[0]
				, baseEdge.second->pair->next->vert->pos.m128_f32[1]
				, baseEdge.second->pair->next->vert->pos.m128_f32[2]
				, baseEdge.second->pair->next->vert->pos.m128_f32[3]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[3]);
		}
		else
		{
			printf("pair : null\n");
		}

	}


	
	eastl::vector<ConvexHullFace_T*> newFaces;
	if (baseEdgeSet.size() > 0)
	{
		newFaces.reserve(baseEdgeSet.size()); // 미리 메모리 확보
	}
	eastl::vector<HE_FACE_T*> outNewFaces;
	CreateVertEdgesTriFan(baseEdgeSet.data(), baseEdgeSet.size(), newExtexsionVert, simplexSet, outNewFaces);

	for (auto const& face : outNewFaces)
	{
			activeFaces.push_back(ToConvexHullFace(face));
			newFaces.push_back(ToConvexHullFace(face));
	}
	printf("생성면 중심점 : %f %f %f %f \n"
		, newExtexsionVert->pos.m128_f32[0]
		, newExtexsionVert->pos.m128_f32[1]
		, newExtexsionVert->pos.m128_f32[2]
		, newExtexsionVert->pos.m128_f32[3]);

	printf("면생성 후\n");
	for (auto const& baseEdge : baseEdgeMap) {
		printf("edge vert %f %f %f %f <- origin vert %f %f %f %f, "
			, baseEdge.second->vert->pos.m128_f32[0]
			, baseEdge.second->vert->pos.m128_f32[1]
			, baseEdge.second->vert->pos.m128_f32[2]
			, baseEdge.second->vert->pos.m128_f32[3]
			, baseEdge.second->originVert->pos.m128_f32[0]
			, baseEdge.second->originVert->pos.m128_f32[1]
			, baseEdge.second->originVert->pos.m128_f32[2]
			, baseEdge.second->originVert->pos.m128_f32[3]);

		if (baseEdge.second->pair != nullptr)
		{
			printf("edge pair vert %f %f %f %f <- origin vert %f %f %f %f,\n"
				, baseEdge.second->pair->vert->pos.m128_f32[0]
				, baseEdge.second->pair->vert->pos.m128_f32[1]
				, baseEdge.second->pair->vert->pos.m128_f32[2]
				, baseEdge.second->pair->vert->pos.m128_f32[3]
				, baseEdge.second->pair->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->originVert->pos.m128_f32[3]);
			printf("next edge pair vert %f %f %f %f <- origin vert %f %f %f %f, \n"
				, baseEdge.second->pair->next->vert->pos.m128_f32[0]
				, baseEdge.second->pair->next->vert->pos.m128_f32[1]
				, baseEdge.second->pair->next->vert->pos.m128_f32[2]
				, baseEdge.second->pair->next->vert->pos.m128_f32[3]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[0]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[1]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[2]
				, baseEdge.second->pair->next->originVert->pos.m128_f32[3]);
		}
		else
		{
			printf("pair : null\n");
		}

	}

	//새로운 구조에 내부점 삭제 및 외부점 탐색
	for (int i = 0; i < newPointSetSize; i++)
	{
		XMVECTOR* point = newPointSet[i];
	
		if (point == extensionPoint)
		{
			continue;
		}

		ConvexHullFace_T* bestFace = nullptr;
		double maxLen =0;

		for (ConvexHullFace_T* face : newFaces)
		{
			XMVECTOR p0 = face->edge->originVert->pos;
			double len = XMVector3Dot(face->norm, *point - p0).m128_f32[0];
			if (len > maxLen)
			{
				maxLen = len;
				bestFace = face;
			}
		}
		
		
		if (bestFace != nullptr) // 0.0 대신 Epsilon 사용
		{
			
			bestFace->pointSet.push_back(point);
			if (maxLen > bestFace->curFarthestPoint.len)
			{
				bestFace->curFarthestPoint.len = maxLen;
				bestFace->curFarthestPoint.point = point;
				bestFace->curFarthestPoint.face = bestFace;
			}
			
		}
	}
	
}

HullInfo_T ConvexHull::GetHullInfo()
{
	HullInfo_T result;
	result.extPointCounter = 0;

	assert(!activeFaces.empty() && "GetHullInfo() called with empty activeFaces list. Check initialization.");	
	result.fathestVert = nullptr;

	double maxLen =0.F;

	for (int i = 0; i < activeFaces.size(); i++)
	{
		result.extPointCounter += activeFaces[i]->curFarthestPoint.len;
		

		if (activeFaces[i]->curFarthestPoint.len > maxLen)
		{
			maxLen = activeFaces[i]->curFarthestPoint.len;
			result.fathestVert = &activeFaces[i]->curFarthestPoint;
			result.fathestVert->face = activeFaces[i];
		}
			
	}

	return result;
}

void ConvexHull::CreateConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size)
{
	
	initSimplex = new Simpelx3D(inVertexArray, size);
	this->simplexSet = &initSimplex->simplexHESet;
	ConvexHullFace_T* face = nullptr;
	//초기 설정
	
	for (int i = 0; i < initSimplex->faceCount; i++)
	{
		face = ToConvexHullFace(initSimplex->simplexHESet.faceSet) + i;
		printf("i %d face %f %f %f %f \n", i, face->norm.m128_f32[0], face->norm.m128_f32[1], face->norm.m128_f32[2], face->norm.m128_f32[3]);
		activeFaces.push_back(face);
		face->curFarthestPoint.len = 0;
	}

	printf("simplex 생성 \n");

	int cnt = 0;
	for (int i = 0; i < size; i++)
	{
		XMVECTOR* point = &inVertexArray[i];
		//printf("point : %f %f %f %f \n", point->m128_f32[0], point->m128_f32[1], point->m128_f32[2], point->m128_f32[3]);
		int faceId = 0; 
		if (!initSimplex->IsPointInSimplex(*point, &faceId))
		{
			cnt++;
			face = ToConvexHullFace(initSimplex->simplexHESet.faceSet) + faceId;
			printf("i:%d faceID: %d vector size : %d push point : %f %f %f %f \n", i,faceId, face->pointSet.size(), point->m128_f32[0], point->m128_f32[1], point->m128_f32[2], point->m128_f32[3]);
			face->pointSet.push_back(point);
			double len = XMVector3Dot(face->norm, *point - face->edge->originVert->pos ).m128_f32[0];
			if (face->curFarthestPoint.len < len)
			{
				face->curFarthestPoint.face = face;
				face->curFarthestPoint.len = len;
				face->curFarthestPoint.point = point;
			}
			
		}
			
	}
	printf("헐생성 초기화 완료 %d \n", cnt);


	//헐 생성 루프
	HullInfo_T CurHullInfo = GetHullInfo();
	while (CurHullInfo.fathestVert != nullptr)
	{
		printf("헐생성 중\n");
		ExtensionHull(CurHullInfo.fathestVert->face, CurHullInfo.fathestVert->point);
		CurHullInfo = this->GetHullInfo();
	}

	printf("final hull face \n");


	
	unsigned int index = 0;

	map<VERTEX_T, unsigned int, VertexCompare> uniqueVertMap;

	for (int i = 0; i < activeFaces.size(); i++)
	{

		VERTEX_T triVertex[3];

		XMStoreFloat4(&triVertex[0].pos, activeFaces[i]->edge->originVert->pos );
		XMStoreFloat4(&triVertex[0].norm, activeFaces[i]->norm);
		triVertex[0].tex = { 0, 0 };
		triVertex[0].textureIdx = 0;

		XMStoreFloat4(&triVertex[1].pos, activeFaces[i]->edge->next->originVert->pos);
		XMStoreFloat4(&triVertex[1].norm, activeFaces[i]->norm);
		triVertex[1].tex = { 0, 0 };
		triVertex[1].textureIdx = 0;

		XMStoreFloat4(&triVertex[2].pos, activeFaces[i]->edge->next->next->originVert->pos);
		XMStoreFloat4(&triVertex[2].norm, activeFaces[i]->norm);
		triVertex[2].tex = { 0, 0 };
		triVertex[2].textureIdx = 0;

		for (int j = 0; j < 3; j++)
		{
			if (uniqueVertMap.count(triVertex[j]) == 0)
			{
				uniqueVertMap[triVertex[j]] = index;
				indexArray.push_back(index);
				vertexArray.push_back(triVertex[j]);
				index++;
			}
			else
			{
				indexArray.push_back(uniqueVertMap[triVertex[j]]);
			}
		}

		//printf("%d face : %f %f %f %f \n", i, activeFaces[i]->norm.m128_f32[0], activeFaces[i]->norm.m128_f32[1], activeFaces[i]->norm.m128_f32[2], activeFaces[i]->norm.m128_f32[3]);

	}
	


}


DirectX::XMVECTOR ConvexHull::Support(XMVECTOR direction, XMMATRIX matTRS)
{
	XMMATRIX inverseTRS = XMMatrixInverse( nullptr , matTRS);

	XMVECTOR localDirection = XMVector3TransformNormal(direction , inverseTRS);

	XMVECTOR farthestVert = {0,};
	float farthestLen = -FLT_MAX;

	for (int i = 0; i < this->vertexArray.size(); i++)
	{
		XMVECTOR vert = XMLoadFloat4(&vertexArray[i].pos);
		XMVECTOR len = XMVector3Dot(vert, localDirection);

		if (len.m128_f32[0] > farthestLen)
		{
			farthestLen = len.m128_f32[0];
			farthestVert = vert;
		}
	}

	farthestVert = XMVector3Transform(farthestVert , matTRS);

	return farthestVert;
}


