#include "MJ_D3D11_ConvexHull.h"
#include "MJ_D3D11_Simplex3D.h"
#include "MJ_D3D11_HalfEdge.h"
#include <EASTL/vector.h>

using namespace DirectX;
using namespace eastl;





ConvexHull::ConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size): Convex(CONVEXHULL)
{
    CreateConvexHull(inVertexArray, size); 
}

ConvexHull::~ConvexHull()
{

}

void ExtensionHull()
{

}

void ConvexHull::CreateConvexHull(DirectX::XMVECTOR* inVertexArray, unsigned int size)
{
	Simpelx3D* simplex = new Simpelx3D();
	simplex->CreateSimplex3D(inVertexArray , size);
	vector<XMVECTOR>outBoundVertexBuffer;

	for (int i = 0; i < size; i++)
	{
		XMVECTOR point = inVertexArray[i];
		point.m128_f32[3] = 0.F;
		if (!simplex->IsPointInSimplex(point))
			outBoundVertexBuffer.push_back(point);
	}
	

}