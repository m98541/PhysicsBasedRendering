#pragma once
#ifndef MJ_D3D11_HALFEDGE_H
#define MJ_D3D11_HALFEDGE_H
#include <DirectXMath.h>

typedef struct HE_VERT_S
{
	DirectX::XMVECTOR pos;
	HE_EDGE_T* edge;
}HE_VERT_T;

typedef struct HE_EDGE_S
{
	HE_VERT_T* vert;
	HE_EDGE_T* pair;
	HE_FACE_T* face;
	HE_EDGE_T* next;

}HE_EDGE_T;

typedef struct HE_FACE_S
{
	HE_EDGE_T* edge;
}HE_FACE_T;


#endif // !MJ_D3D11_HALFEDGE_H

