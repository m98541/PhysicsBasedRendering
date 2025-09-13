#ifndef MJ_D3D11_CONVEXHULL_H
#define MJ_D3D11_CONVEXHULL_H
#include "VertexType.h"

enum ConvexType
{
	SPHERE,
	CYILINDER,
	CUBE,
	CAPSULE,
	CONVEXHULL
};

class Convex
{
	public:
		Convex(const ConvexType type) : convexType(type) {};
	
		virtual ~Convex() {};
		//const ConvexType& getType() const { return convexType; }
		//virtual const DirectX::XMVECTOR& getCenter() const = 0;
		//virtual const DirectX::XMVECTOR getFarthestPoint(const  DirectX::XMVECTOR direction, const DirectX::XMMATRIX& transform) const = 0;

	private:
		const ConvexType convexType;
};





#endif // !MJ_D3D11_CONVEXHULL_H
