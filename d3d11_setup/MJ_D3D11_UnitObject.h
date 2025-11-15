#include "VertexType.h"
#include "MJ_D3D11_OBJLoader.h"
#include "MJ_D3D11_ConvexHull.h"
#include "MJ_D3D11_Collision.h"

namespace UnitObject
{
	
	typedef struct UNIT_DESC_S
	{
		ID3D11Device* Dev;
		ID3D11DeviceContext* DevCon;
		ID3D10Blob* vsShader;
		MJD3D11OBJ_HANDLE_t* objHandle;
		CapsuleCollision::CAPSULE_T mapCollider;
		ConvexHull* objCollider;
	}UNIT_DESC_T;

	class UnitObj
	{
	public:
		
		UnitObj(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, ID3D10Blob* vsShader, MJD3D11OBJ_HANDLE_t* objHandle, CapsuleCollision::CAPSULE_T mapCollider, ConvexHull* objCollider);
		~UnitObj();
		CapsuleCollision::CAPSULE_T mapCollider;
		
		ConvexHull* objCollider;

		void setPos(DirectX::XMFLOAT4 pos);
		void setRotate(double rotate);
		void setScale(DirectX::XMFLOAT4 scale);

		DirectX::XMFLOAT4 getPos();
		DirectX::XMFLOAT4 getScale();
		double getRotate();

		void DrawObject();
		void DrawCollider();
		

	private:
		//object file info
		MJD3D11OBJ_HANDLE_t* objHandle;
		MJD3D11OBJ_HANDLE_t* colliderObjHandle;

		//dx11 shader info
		ID3D11Device* Dev;
		ID3D11DeviceContext* DevCon;
		ID3D10Blob* vsShader;

		DirectX::XMFLOAT4 pos;
		double rotate;
		DirectX::XMFLOAT4 scale;

		DirectX::XMMATRIX TRS;
		ID3D11Buffer* modelBuffer;
		
		ID3D11ShaderResourceView* textureSRV;
		ID3D11SamplerState* sampler;

		void LoadColliderView();
		//set 함수 마지막에 무조건 호출!
		ID3D11RasterizerState* pSolidRS;       // 기본(채우기) 상태
		ID3D11RasterizerState* pWireframeRS;   // 와이어프레임(선) 상태

		void updateTRS();
		
	};


}