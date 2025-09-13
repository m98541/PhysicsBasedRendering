#pragma once
#ifndef CONVEXHULL_H
#define CONVEXHULL_H
#include "MJ_D3D11_Convex.h"
#include <EASTL/vector.h>




class ConvexHull : public Convex
{
	public:
		eastl::vector<DirectX::XMVECTOR> vertexArray;
		eastl::vector<unsigned int> indexArray;//EPA 구현시 필요

		ConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size);
		~ConvexHull();
		//DirectX::XMVECTOR Support(DirectX::XMVECTOR direction); GJK 구현시 필요
	private:
		void CreateConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size);

};

#endif // !CONVEXHULL_H
