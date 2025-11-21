#ifndef MJ_D3D11_EPA_H
#define MJ_D3D11_EPA_H
#include <DirectXMath.h>
#include <EASTL/priority_queue.h>
#include <EASTL/functional.h>
#include <EASTL/vector.h>
#include "MJ_D3D11_GJK.h"

/*
EPA FACE 구조는 이후 메모리 풀 관리 시스템 개발 후 
Half Edge 구조로 변경 예정 -> 현재 Half Edge 구조는 메모리 할당 비용이 너무커(현재는 초기 로딩중에서만 사용) 
hot loop 에서 작동하기에는 적절하지 않음
*/

typedef struct EPA_FACE_S EPA_FACE_T;

struct EPA_FACE_S
{
	DirectX::XMVECTOR points[3];
	DirectX::XMVECTOR norm;
	double distance;
};


typedef struct EPA_INFO_S
{
	DirectX::XMVECTOR direction;
	double distance;

}EPA_INFO_T;

EPA_INFO_T CreateEPAInfo(gjkSimplex& gjkInfo, ConvexHull* A, DirectX::XMMATRIX matTRS_A, ConvexHull* B, DirectX::XMMATRIX matTRS_B);



#endif // !MJ_D3D11_EPA_H