#pragma once
#ifndef VERTEXTYPE_H
#define VERTEXTYPE_H
#include <DirectXMath.h>

typedef struct VERTEX_S
{
	DirectX::XMFLOAT4 pos;
	DirectX::XMFLOAT2 tex;
	DirectX::XMFLOAT4 norm;
	int textureIdx;

}VERTEX_T;

typedef struct TRIPLANE_S
{
	DirectX::XMFLOAT4 Vertex[3];
	DirectX::XMFLOAT4 Norm;
}TRIPLANE_T;

typedef struct TRIPOLY_S
{
	DirectX::XMFLOAT4 Vertex[3];
	DirectX::XMVECTOR Centroid;
	DirectX::XMVECTOR MaxCoord;
	DirectX::XMVECTOR MinCoord;
}TRIPOLY_T;




#endif // !VERTEXTYPE_H
