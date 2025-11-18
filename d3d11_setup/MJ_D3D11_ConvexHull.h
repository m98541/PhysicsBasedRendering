#pragma once
#ifndef CONVEXHULL_H
#define CONVEXHULL_H
#include "MJ_D3D11_Convex.h"
#include "MJ_D3D11_HalfEdge.h"
#include "MJ_D3D11_Simplex3D.h"
#include <EASTL/vector.h>
#include <cassert>

typedef struct ConvexHullVert_S ConvexHullVert_T;
typedef struct ConvexHullFace_S ConvexHullFace_T;

struct ConvexHullVert_S
{
	DirectX::XMVECTOR* point;
	ConvexHullFace_T* face;//point 에서 무조건 보이는 면
	double len;//면과 점 거리
};

struct ConvexHullFace_S : public HalfEdge::HE_FACE_T
{
	ConvexHullVert_T curFarthestPoint;
	eastl::vector<DirectX::XMVECTOR*> pointSet;
};

typedef struct HullInfo_S
{
	int extPointCounter;
	ConvexHullVert_T* fathestVert;
}HullInfo_T;

class ConvexHull : public Convex
{
	public:
		eastl::vector<VERTEX_T> vertexArray;
		eastl::vector<unsigned int> indexArray;//EPA 구현시 필요
		ConvexHull();
		ConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size);
		~ConvexHull();
		DirectX::XMVECTOR Support(DirectX::XMVECTOR direction ,DirectX::XMMATRIX matTRS); //GJK 구현시 필요
	private:
		eastl::vector<ConvexHullFace_T*> activeFaces;
		
		void ExtensionHull(ConvexHullFace_T* startFace, DirectX::XMVECTOR* extensionPoint);
		void CreateConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size);
	
		HullInfo_T GetHullInfo();
		HalfEdge::HE_SET_T* simplexSet;
		Simpelx3D* initSimplex;
		
};

#endif // !CONVEXHULL_H
