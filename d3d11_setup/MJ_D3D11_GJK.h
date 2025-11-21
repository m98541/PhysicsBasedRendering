#pragma once
#ifndef MJ_D3D11_GJK_H
#define MJ_D3D11_GJK_H
#include "MJ_D3D11_ConvexHull.h"


class gjkSimplex
{
	public:
		DirectX::XMVECTOR points[4];
		DirectX::XMVECTOR faces[4][3];//3 tetrahedron format
		int level;// 0 point , 1 line , 2 tri , 3 tetrahedron
		gjkSimplex();
		~gjkSimplex();

		void addPoint(DirectX::XMVECTOR point);

		void SetPoints(DirectX::XMVECTOR point);
		void SetPoints(DirectX::XMVECTOR point0 , DirectX::XMVECTOR point1);
		void SetPoints(DirectX::XMVECTOR point0 , DirectX::XMVECTOR point1 , DirectX::XMVECTOR point2);
		void SetPoints(DirectX::XMVECTOR point0, DirectX::XMVECTOR point1, DirectX::XMVECTOR point2, DirectX::XMVECTOR point3);
		
};


bool gjkCollisionCheck(ConvexHull* A,DirectX::XMMATRIX matTRS_A,ConvexHull* B, DirectX::XMMATRIX matTRS_B, gjkSimplex& simplex);

#endif // !MJ_D3D11_GJK_H

