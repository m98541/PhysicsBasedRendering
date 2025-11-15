#include <windows.h>
#include <stdio.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dtypes.h>
#include <DirectXMath.h>
#include <EASTL/vector.h>


#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif // _DEBUG
#pragma comment(lib , "dxgi.lib")
#pragma comment(lib , "d3d11.lib")
#pragma comment(lib , "d3dcompiler.lib")


/*
라이브러리 이름 분류
BmpFileIO 다음과 같이 기능 이름만 있는경우 => standard(?) 라이브러리 지원 모든 윈도우즈에서 사용가능

MJ_D3D11_ 다음의 표시가 붙은 경우(비표준 라이브러리(?)) directX11 라이브러리에 종속  directX11 지원시에만 사용가능
*/
#include "BmpFileIO.h"
#include "OBJFileIO.h"
#include "MJ_D3D11_OBJLoader.h"
#include "MJ_D3D11_BasicCam.h"
#include "MJ_D3D11_Collision.h"
#include "MJ_D3D11_WorldMap.h"
#include "MJ_D3D11_HalfEdge.h"
#include "MJ_D3D11_ConvexHull.h"
#include "MJ_D3D11_UnitObject.h"

#define SCREEN_SIZE_WIDTH 1920
#define SCREEN_SIZE_HEIGHT 1080

using namespace DirectX;
using namespace CapsuleCollision;
using namespace Map;
IDXGISwapChain* swapChain; // swap chain interface pointer
/*
	swap 체인 = swap 버퍼 -> 이러한 체인에 대한 인터페이스 포인터
	Direct3D를 기반으로 하는 DXGI의 일부
*/
ID3D11Device* dev; // d3d device interface pointer
/*
	dev 는 장치에 대한 포인터 
	d3d에서 장치는 가상표현 객체 => ID3D11Device 라는 COM 객체를 만드는 것 
*/
ID3D11DeviceContext* devCon; // d3d device context pointer
/*
	장치 컨텍스트 , GPU 및 렌더링 파이프라인 관리
*/

ID3D11RenderTargetView* backBuffer;

ID3D11DepthStencilView* depthBuffer;

IDXGIFactory1* pFactory = nullptr;

IDXGIAdapter1* pAdapter = nullptr;

ID3D11VertexShader* pVS; // main vertexShader pointer 

ID3D11PixelShader* pPS; // main pixelShader pointer

ID3D10Blob* VS, * PS;

ID3D11Buffer* pVBuffer;

ID3D11Buffer* pIBuffer;


ID3D11InputLayout* pLayout;

ID3D11Buffer* pTRBuffer = nullptr;

ID3D11Buffer* pCamBuffer = nullptr;
ID3D11Buffer* pModelBuffer = nullptr;


ID3D11ShaderResourceView* pTextureSRV = nullptr;

ID3D11SamplerState* pSampler = nullptr;


OBJFILE_DESC_T objDesc;
OBJFILE_BUFFER_T* objBuffer = nullptr;


MJD3D11OBJ_HANDLE_t* objHandle;

BasicCam* singleCam;
BasicCam* singleNextCam;

constexpr float gravity = 0.F;

int mouseMoveOn;
XMVECTOR mouseMoveVector;

float camAccV = 0.F;
float camAccRLV = 0.F;//left right speed scale
float camAccR = 0.F;

bool camJumpState = false;
float camJumpSpeed = 0.5F;

ULONGLONG jumpStartTick;
ULONGLONG jumpCurTick;

struct Model
{
	XMMATRIX m;
};
struct VP
{
	
	XMMATRIX view;
	XMMATRIX proj;
};

eastl::vector<UnitObject::UnitObj*> unitManager;
UnitObject::UNIT_DESC_T globalCat;

D3D_FEATURE_LEVEL featureLevelarr[] = {
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
	D3D_FEATURE_LEVEL_9_2,
	D3D_FEATURE_LEVEL_9_1,
};

int mouseTick = 0;
int frameTick = 0;

void InitD3D(HWND hWnd); // sets up & init D3D
void InitData(void);//verts data mapping memory
void CleanD3D(void); // closes D3D & release mem
void InitPipeline();
void SelectDisplayAdapter(void); 
void RenderFrame(void);
void init_DepthBuffer(void);
void init_MVPtrans(void);
void CreateShaderResourceViewFromBMPFile(ID3D11Device* device, const char* fileName, UINT bmp_format, ID3D11ShaderResourceView** Texture_SRV);


bool BoxViewMode = false;
int BoxDrawDepth = 0;


void* __cdecl operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	// 정렬된 메모리 할당 (Windows 기준)
	return _aligned_malloc(size, alignment);
}

void* __cdecl operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	// 일반 할당
	return malloc(size);
}
/*
void __cdecl operator delete[](void* ptr)
{
	_aligned_free(ptr);
}
*/
void __cdecl operator delete[](void* ptr)
{
	free(ptr);
}





void Draw_Box(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, XMVECTOR max, XMVECTOR min)
{
	VERTEX_T boxVertices[24]; // 12 edges × 2 vertices

	XMFLOAT4 vMin, vMax;
	XMStoreFloat4(&vMin, min);
	XMStoreFloat4(&vMax, max);

	// 8 corners of the box
	XMFLOAT4 corners[8] = {
		{vMin.x, vMin.y, vMin.z, 1.0f}, // 0
		{vMax.x, vMin.y, vMin.z, 1.0f}, // 1
		{vMax.x, vMax.y, vMin.z, 1.0f}, // 2
		{vMin.x, vMax.y, vMin.z, 1.0f}, // 3
		{vMin.x, vMin.y, vMax.z, 1.0f}, // 4
		{vMax.x, vMin.y, vMax.z, 1.0f}, // 5
		{vMax.x, vMax.y, vMax.z, 1.0f}, // 6
		{vMin.x, vMax.y, vMax.z, 1.0f}  // 7
	};

	// 12 edges (pairs of indices)
	int edgeIndices[24] = {
		0,1, 1,2, 2,3, 3,0, // bottom face
		4,5, 5,6, 6,7, 7,4, // top face
		0,4, 1,5, 2,6, 3,7  // vertical edges
	};

	for (int i = 0; i < 24; ++i)
	{
		boxVertices[i].pos = corners[edgeIndices[i]];
		boxVertices[i].tex = XMFLOAT2(0.5f, 0.5f); // not used
		boxVertices[i].norm = XMFLOAT4(10000.F, 0.F , 0.F, 1.0f); // dummy normal
		boxVertices[i].textureIdx = 1; // dummy texture index
	}

	// Create vertex buffer
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(VERTEX_T) * 24;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = boxVertices;

	ID3D11Buffer* lineBuffer = nullptr;
	HRESULT hr = Dev->CreateBuffer(&bd, &initData, &lineBuffer);
	if (FAILED(hr)) return;

	// Bind buffer and draw
	UINT stride = sizeof(VERTEX_T);
	UINT offset = 0;
	DevCon->IASetVertexBuffers(0, 1, &lineBuffer, &stride, &offset);
	DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	DevCon->Draw(24, 0);

	if (lineBuffer) lineBuffer->Release();
}

void DrawMapBox(BVtree::BvNode* root, int depth)
{
	if (root == nullptr) return;

	if (depth == 0)
	{
		Draw_Box(dev, devCon, root->mMaxCoord, root->mMinCoord);
		return;
	}
	else if (root->mLeft == nullptr && root->mRight == nullptr)
	{
		Draw_Box(dev , devCon , root->mMaxCoord , root->mMinCoord);
		return;
	}
	else
	{
		DrawMapBox(root->mLeft,depth - 1);
		DrawMapBox(root->mRight,depth - 1);
	}

}


LRESULT CALLBACK WndProc(HWND hWnd,UINT iMessage,WPARAM wParam,LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance ,HINSTANCE hPorevInstance, LPSTR lpCmdLine ,int nCmdShow){

	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;	


	ZeroMemory(&WndClass , sizeof(WNDCLASSEX));

	

	WndClass.cbSize = sizeof(WNDCLASSEX);
	WndClass.hCursor = LoadCursor(NULL , IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
	WndClass.hInstance = hInstance;

	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = L"first";
	WndClass.lpszMenuName = NULL;
	
	WndClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClassEx(&WndClass);


	hWnd = CreateWindow(L"first" ,L"firstWindows",
		WS_OVERLAPPEDWINDOW,
		300 , 
		300 , 
		SCREEN_SIZE_WIDTH,
		SCREEN_SIZE_HEIGHT,
		NULL,
		(HMENU)NULL,
		hInstance ,
		NULL );
	
	ShowWindow(hWnd , nCmdShow);


	

	ShowCursor(TRUE);
	

	SelectDisplayAdapter();
	singleCam = new BasicCam();
	singleNextCam = new BasicCam();
	InitD3D(hWnd);
	InitPipeline();


	


	OBJFILE_DESC_T collTestObject;
	OBJFILE_BUFFER_T* collTestObjBuf;

	ParseOBJFile(&collTestObject , "de_dust2.obj");
	ReadOBJFile(&collTestObject , &collTestObjBuf);

	VERTEX_T* collVertexBuffer = (VERTEX_T*)malloc(sizeof(VERTEX_T) * collTestObjBuf->objectBufferLen);

	XMMATRIX tempMat = { {1.F, 0.F , 0.F , 0.F} , {0.F, 0.F , -1.F , 0.F} ,  {0.F, 1.F ,0.F , 0.F} ,  {0.F, 0.F , 0.F , 1.F} };
	tempMat = XMMatrixTranspose(tempMat);

	XMFLOAT4* testMapBuffer = (XMFLOAT4*)malloc(sizeof(XMFLOAT4) * collTestObjBuf->objectBufferLen);
	
	for (int i = 0; collTestObjBuf != NULL && i < collTestObjBuf->objectBufferLen; i++)
	{
		memcpy(&collVertexBuffer[i].pos, collTestObjBuf->objectBuffer + i * 6, sizeof(float) * 4);

		XMVECTOR temp = XMLoadFloat4(&collVertexBuffer[i].pos);
		collVertexBuffer[i].pos.x =XMVector4Dot( tempMat.r[0], temp).m128_f32[0];
		collVertexBuffer[i].pos.y = XMVector4Dot(tempMat.r[1], temp).m128_f32[0];
		collVertexBuffer[i].pos.z = XMVector4Dot(tempMat.r[2], temp).m128_f32[0];
		collVertexBuffer[i].pos.w = 1.F;

		testMapBuffer[i] = collVertexBuffer[i].pos;
	}

	MapObject* testMapObj =new MapObject(testMapBuffer, collTestObjBuf->objectBufferLen);
	testMapObj->MapBuild();


	MJD3D11LoadOBJ(dev,devCon,VS,&objHandle , "de_dust2.obj");
	init_MVPtrans();

	POINT initCursor = { SCREEN_SIZE_WIDTH / 2   ,SCREEN_SIZE_HEIGHT / 2 };

	ClientToScreen(hWnd, &initCursor);
	SetCursorPos(initCursor.x, initCursor.y);
	mouseMoveVector = {0.F, 0.F, 0.F, 0.F};
	float radius = 30.F;
	float height = 30.F;
	CAPSULE_T initCapsule = {
		{0.F , height + radius, 0.F, 1.F},
		{0.F , radius , 0.F , 1.F},
		{0.F , height, 0.F , 0.F},
		radius
	};

	singleCam->Element.pos = {-376,84,-1145 , 1};
	*singleNextCam = *singleCam;
	XMVECTOR initH= { 0 , height , 0 , 0 };

	initCapsule.head = singleCam->Element.pos;

	initCapsule.foot = singleCam->Element.pos - initH;

	CapsuleCollider nextColider(initCapsule);
	CapsuleCollider collider(initCapsule);
	collider.Collider.head = singleCam->Element.pos;
	collider.Collider.foot = singleCam->Element.pos;
	collider.Collider.foot.m128_f32[1] -= height;
	ULONGLONG startTick = GetTickCount64();
	ULONGLONG curTick;




	OBJFILE_DESC_T catModelObject;
	OBJFILE_BUFFER_T* catModelObjectBuf;
	MJD3D11OBJ_HANDLE_t* catOBJHandle;
	ParseOBJFile(&catModelObject, "cat.obj");
	ReadOBJFile(&catModelObject, &catModelObjectBuf);
	printf("read cat obj\n");
	ConvexHull* catConvexHull = new ConvexHull((XMVECTOR*)catModelObjectBuf->vertBuffer, catModelObjectBuf->vertBufferLen);

	float catRadius = 10.F;
	float catHeight = 10.F;
	MJD3D11LoadOBJ(dev,devCon,VS,&catOBJHandle , "cat.obj");
	CAPSULE_T catCapsule = {
		{0.F , catHeight + catRadius, 0.F, 1.F},
		{0.F , catRadius , 0.F , 1.F},
		{0.F , catHeight, 0.F , 0.F},
		catRadius
	};


	for (int i = 0; i < catConvexHull->vertexArray.size(); i++)
	{
		printf("vert : %f %f %f %f \n"
			, catConvexHull->vertexArray[i].pos.x
			, catConvexHull->vertexArray[i].pos.y
			, catConvexHull->vertexArray[i].pos.z
			, catConvexHull->vertexArray[i].pos.w);
	}

	for (int i = 0; i < catConvexHull->indexArray.size(); i += 3)
	{
		printf("idx : %d %d %d \n"
			, catConvexHull->indexArray[i]
			, catConvexHull->indexArray[i + 1]
			, catConvexHull->indexArray[i + 2]
		);
	}
	
	globalCat = {
		dev,devCon,VS,
		catOBJHandle,catCapsule,catConvexHull
	};


	while (true)
	{		
		
		if (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
		{
		
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		
			if (Message.message == WM_QUIT) break;
			initCursor = { SCREEN_SIZE_WIDTH / 2   ,SCREEN_SIZE_HEIGHT / 2 };
			ClientToScreen(hWnd, &initCursor);
			SetCursorPos(initCursor.x, initCursor.y);
			
		}
		else
		{

			frameTick++;
			curTick = GetTickCount64();
			if (curTick - startTick > 1000)
			{
				//printf("fps: %u \n", frameTick);
				frameTick = 0;
				startTick = curTick;
			}
			// 셰이더에 바인딩
			devCon->VSSetConstantBuffers(0, 1, &pCamBuffer);
			devCon->VSSetConstantBuffers(1, 1, &pModelBuffer);
			devCon->ClearDepthStencilView(depthBuffer, D3D11_CLEAR_DEPTH, 1.0f, 0);
			
			Model model;
			VP cam;

			singleNextCam->MoveFrontBack(camAccV);
			singleNextCam->MoveLeftRight(camAccRLV);
			
			

			if (camJumpState)
			{
				singleNextCam->Element.pos.m128_f32[1] += camJumpSpeed;
				singleNextCam->Element.at.m128_f32[1] += camJumpSpeed;
				
				jumpCurTick = GetTickCount64();
				if (jumpCurTick - jumpStartTick > 150)
				{
					camJumpState = false;
					jumpCurTick = 0;
					jumpStartTick = 0;
				}
					
				
			}
			else
			{
				singleNextCam->Element.pos.m128_f32[1] -= gravity;
				singleNextCam->Element.at.m128_f32[1] -= gravity;
			}
		
			
			

			singleCam->TracballRoateNormVector(mouseMoveVector, (3.141592F / 180.F) * 20.F);
			singleNextCam->TracballRoateNormVector(mouseMoveVector, (3.141592F / 180.F) * 20.F);
			
			nextColider.Collider.head = singleNextCam->Element.pos;
			nextColider.Collider.foot = singleNextCam->Element.pos;
			nextColider.Collider.foot.m128_f32[1] -= height;
			
			collider.nextMove = { 0 , 0 , 0 , 0 };
	
			eastl::vector<SWEEP_HIT_T> swpHitSet;

			XMVECTOR move = singleNextCam->Element.pos - singleCam->Element.pos;
			XMVECTOR remainMove = move;

			
			for (int i = 0; i < 3; i++)
			{
				int next = 0;
				bool swpTest = testMapObj->IsMapSwpCollisionDetect(collider, nextColider.Collider , swpHitSet);
				
		
				if (swpTest)
				{	
					

				
					//printf("%d hitTime %f %d \n", i, swpHitSet[0].hitTime, swpHitSet[0].element);
					nextColider.CollisionSlide(swpHitSet[0].normal, remainMove);
					singleNextCam->Element.pos = singleCam->Element.pos + nextColider.nextMove;
					singleNextCam->Element.at = singleCam->Element.at + nextColider.nextMove;

					
					nextColider.Collider.head = singleNextCam->Element.pos;
					nextColider.Collider.foot = nextColider.Collider.head;
					nextColider.Collider.foot.m128_f32[1] -= height;
					
					remainMove = nextColider.nextMove;
				}
				else
				{
					break;
				}
	
				for (int j = 0; j < swpHitSet.size(); j++)
				{

					if (nextColider.TriAngleCollisionTest(swpHitSet[j].tri))
					{

						//printf("겹침 normal : %f %f %f %f \n", swpHitSet[0].normal.m128_f32[0], swpHitSet[0].normal.m128_f32[1], swpHitSet[0].normal.m128_f32[2], swpHitSet[0].normal.m128_f32[3]);
						singleNextCam->Element.pos = singleNextCam->Element.pos + swpHitSet[j].normal * swpHitSet[j].penetrationDepth;
						singleNextCam->Element.at = singleNextCam->Element.at + swpHitSet[j].normal * swpHitSet[j].penetrationDepth;


						nextColider.Collider.head = singleNextCam->Element.pos;
						nextColider.Collider.foot = nextColider.Collider.head;
						nextColider.Collider.foot.m128_f32[1] -= height;

						remainMove = singleNextCam->Element.pos - singleCam->Element.pos;
					}
				}
				
			}

			



			
			collider.Collider = nextColider.Collider;
			
			*singleCam = *singleNextCam;
			
			

			D3D11_MAPPED_SUBRESOURCE mapped;


			model.m = { {1.F, 0.F , 0.F , 0.F} , {0.F, 0.F , -1.F , 0.F} ,  {0.F, 1.F ,0.F , 0.F} ,  {0.F, 0.F , 0.F , 1.F} };
			cam.view = XMMatrixLookAtLH(singleCam->Element.pos, singleCam->Element.at, singleCam->Element.up);
			cam.proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0F / 600.0F, 0.01F, 8000.F);

			devCon->Map(pCamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			memcpy(mapped.pData, &cam, sizeof(cam));
			devCon->Unmap(pCamBuffer, 0);


			
			devCon->Map(pModelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
			memcpy(mapped.pData, &model, sizeof(model));
			devCon->Unmap(pModelBuffer, 0);

			RenderFrame();

			if (BoxViewMode)
			{

				model.m = { {1.F, 0.F , 0.F , 0.F} , {0.F, -1.F , 0.F , 0.F} ,  {0.F, 0.F ,1.F , 0.F} ,  {0.F, 0.F , 0.F , 1.F} };
				cam.view = XMMatrixLookAtLH(singleCam->Element.pos, singleCam->Element.at, singleCam->Element.up);
				cam.proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0F / 600.0F, 0.01F, 8000.F);

				devCon->Map(pModelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
				memcpy(mapped.pData, &model, sizeof(model));
				devCon->Unmap(pModelBuffer, 0);
				DrawMapBox(testMapObj->mBoxVolumeTree, BoxDrawDepth);
			}
			

			for (int i = 0; i < unitManager.size(); i++)
			{
			



				unitManager[i]->DrawObject();
				unitManager[i]->DrawCollider();

			}
			
			swapChain->Present(0, 0);


			
		}
		//game loop;

		
	}
	CleanD3D();
	return Message.wParam;

	


}

void pushUnit(XMVECTOR camPos)
{
	UnitObject::UnitObj* catOBJ = new UnitObject::UnitObj(globalCat.Dev, globalCat.DevCon, globalCat.vsShader, globalCat.objHandle, globalCat.mapCollider, globalCat.objCollider);
	XMFLOAT4 catScale = { 5.F , 5.F ,5.F , 1.F };
	catOBJ->setScale(catScale);
	XMFLOAT4 catPos;
	XMStoreFloat4(&catPos, singleCam->Element.pos);
	catOBJ->setPos(catPos);
	unitManager.push_back(catOBJ);
	printf("cat push! pos : %f %f %f \n" , catPos.x , catPos.y , catPos.z);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	
	POINT cursor;
	static int mouseInWindow = 0;
	
	float speed = 1.f;
	
	switch (iMessage)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CHAR:
		switch (wParam)
		{
		//대각선으로 움직이면 속도 더 높은거는 유지 ㄱㄱ 
		case 'a':
		case 'A':
			camAccRLV = -1.F * speed;// 0.00001F 해당 부분은 맵 사이즈에 혹은 렌더링 오브젝트 스케일 따라 유동적 
			break;
		case 'd':
		case 'D':
			camAccRLV = 1.F * speed;
			break;
		case 'w':
		case 'W':
			camAccV = 1.F * speed;
			break;
		case 's':
		case 'S':
			camAccV = -1.F * speed;
			break;
		case 'p':
		case 'P':
			printf("pos %f %f %f \n" , 
				singleCam->Element.pos.m128_f32[0],
				singleCam->Element.pos.m128_f32[1],
				singleCam->Element.pos.m128_f32[2]);
			break;
		case 'V':
		case 'v':
			BoxViewMode = !BoxViewMode;
			break;

		case 'B':
		case 'b':
			BoxDrawDepth = (BoxDrawDepth + 1) % 15;
			break;

		case 'c':
		case 'C':
			pushUnit(singleCam->Element.pos);
			break;
		case VK_SPACE:
			if(!camJumpState)
				jumpStartTick = GetTickCount64();
			camJumpState = true;
			break;
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'a':
		case 'A':
			camAccRLV = 0;
			break;
		case 'd':
		case 'D':
			camAccRLV = 0;
			break;
		case 'w':
		case 'W':
			camAccV = 0;
			break;
		case 's':
		case 'S':
			camAccV = 0;
			break;
		default:
			break;
		}
		break;

	case WM_MOUSEMOVE:
		GetCursorPos(&cursor);
		ScreenToClient(hWnd, &cursor);
		mouseMoveVector = { ((float)cursor.x / SCREEN_SIZE_WIDTH) * 2.F - 1.F ,-1 * ((float)cursor.y / SCREEN_SIZE_HEIGHT) * 2.F + 1.F , 0.F ,0.F };
		break;

	default:
		break;
	}
	// default window procedure  
	// => app 에서 처리안된 메시지 중 기본적인거(?? 아마 window에서 default 로  미리 지정된?) 처리
	// 주요 처리 메세지 예) WM_CLOSE , WM_DESTROY , WM_PAINT ,WM_KEYDOWN , WM_MOUSEMOVE 등등
	// 주요 처리 메세지의 경우 micro 공식 문서 참조 하여 사용자 정의 필요...
	
	//do while 로 변경필요
	



	return (DefWindowProc(hWnd , iMessage , wParam , lParam));

}



void InitD3D(HWND hWnd)
{
	//D3D init
	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));
	D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	UINT DeviceAndSwapchainFlag = 0;

	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferDesc.Width = SCREEN_SIZE_WIDTH;
	scd.BufferDesc.Height = SCREEN_SIZE_HEIGHT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 4;
	scd.Windowed = TRUE;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	D3D11CreateDeviceAndSwapChain(
		pAdapter,
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		DeviceAndSwapchainFlag,//
		featureLevelarr,
		sizeof(featureLevelarr) / sizeof(D3D_FEATURE_LEVEL),
		D3D11_SDK_VERSION,
		&scd,
		&swapChain,
		&dev,
		&featureLevel,
		&devCon
	);

	/*
	pAdapter 매개 변수를 NULL이 아닌 값으로 설정하는 경우 DriverType 매개 변수도 D3D_DRIVER_TYPE_UNKNOWN 값으로 설정해야 합니다. 
	pAdapter 매개 변수를 NULL이 아닌 값으로 설정하고 DriverType 매개 변수를 D3D_DRIVER_TYPE_HARDWARE 값으로 설정하면
	D3D11CreateDeviceAndSwapChain은 E_INVALIDARG HRESULT를 반환합니다.
	출처:https://learn.microsoft.com/ko-kr/windows/win32/api/d3d11/nf-d3d11-d3d11createdeviceandswapchain
	*/

	
	// set the render target
	ID3D11Texture2D* pBackBuffer;

	// universally unique identifier of (ID3D11Texture2D)
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	dev->CreateRenderTargetView(pBackBuffer , NULL , &backBuffer);
	pBackBuffer->Release();

	init_DepthBuffer();
	devCon->OMSetRenderTargets(1 , &backBuffer , depthBuffer);

	//set the viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport , sizeof(D3D11_VIEWPORT));

	viewport = {
		0.F , 0.F,  // top left x y  
		SCREEN_SIZE_WIDTH , SCREEN_SIZE_HEIGHT , // width  ,height 
		0.F , 1.F // depth min max
	};
	
	D3D11_RASTERIZER_DESC rasterDesc = {};
	rasterDesc.CullMode = D3D11_CULL_BACK;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	ID3D11RasterizerState* rasterState = nullptr;
	dev->CreateRasterizerState(&rasterDesc, &rasterState);
	devCon->RSSetState(rasterState);


	//RSSetViewports => RS : Resterizer Stage , Set Viewport
	devCon->RSSetViewports(1, &viewport);

}

void CleanD3D(void)
{
	
	pVS->Release();
	pPS->Release();
	swapChain->SetFullscreenState(FALSE , NULL);
	pFactory->Release();
	pAdapter->Release();
	swapChain->Release();
	backBuffer->Release();
	dev->Release();
	devCon->Release();
}

void RenderFrame(void)
{
	SYSTEMTIME curTime;
	GetLocalTime(&curTime);
	float color = 0.4F;  //curTime.wSecond/60.F;
	const float colors[4] = {color, color, color, 1.0F};
	devCon->ClearRenderTargetView(backBuffer, colors);

	MJD3D11DrawOBJ(devCon , objHandle);
	
}

void SelectDisplayAdapter(void)
{
	DXGI_ADAPTER_DESC1 AdapterDesc;
	int adapter_num = 0;

	size_t max_mem = 0;
	int max_adapter = 0;
	CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);
	while (SUCCEEDED(pFactory->EnumAdapters1(adapter_num, &pAdapter)))
	{
		pAdapter->GetDesc1(&AdapterDesc);
		wprintf(L"%d : %ls \n", adapter_num, AdapterDesc.Description);

		if (AdapterDesc.DedicatedVideoMemory > max_mem)
		{
			max_mem = AdapterDesc.DedicatedVideoMemory;
			max_adapter = adapter_num;
		}
		++adapter_num;
	}
	

	pFactory->EnumAdapters1(max_adapter, &pAdapter);
	printf("선택된 디스플레이 어뎁터 : ");
	pAdapter->GetDesc1(&AdapterDesc);
	wprintf(L"adapter %ls \n", AdapterDesc.Description);
	printf("adapter VendorId %u \n", AdapterDesc.VendorId);
	printf("adapter DeviceId %u \n", AdapterDesc.DeviceId);
	printf("adapter SubSysId %u \n", AdapterDesc.SubSysId);
	
}

void InitPipeline()
{
	printf("PipeLine Init\n");
	

	if (SUCCEEDED(D3DCompileFromFile(L"VertexShader.hlsl", 0, 0,"main", "vs_5_0", 0, 0, &VS, 0)))
	{
		dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	
		devCon->VSSetShader(pVS, 0, 0);
		printf("vert shader compile succeded\n");
	}
	else
	{
		printf("vert shader compile failed\n");
		return;
	}


	if (SUCCEEDED(D3DCompileFromFile(L"PixelShader.hlsl", 0, 0, "main", "ps_5_0", 0, 0, &PS, 0)))
	{
		dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);
		
		devCon->PSSetShader(pPS, 0, 0);
		printf("pixel shader compile succeded\n");
	}
	else
	{
		printf("pixel shader compile failed\n");
		return;

	}

}




void init_MVPtrans(void)
{

	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(VP);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	dev->CreateBuffer(&cbd, nullptr, &pCamBuffer);

	D3D11_BUFFER_DESC cbdModel = {};
	cbdModel.Usage = D3D11_USAGE_DYNAMIC;
	cbdModel.ByteWidth = sizeof(Model);
	cbdModel.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbdModel.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	dev->CreateBuffer(&cbdModel, nullptr, &pModelBuffer);


	Model modelMat;
	VP cam;
	modelMat.m = { {1.F, 0.F , 0.F , 0.F} , {0.F, 0.F , -1.F , 0.F} ,  {0.F, 1.F , 0.F , 0.F} ,  {0.F, 0.F , 0.F , 1.F} };
	cam.view = XMMatrixLookAtLH(singleCam->Element.pos, singleCam->Element.at, singleCam->Element.up);
	cam.proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 800.0f / 600.0f, 0.1f, 3000.0f);

	
	D3D11_MAPPED_SUBRESOURCE mapped;
	devCon->Map(pModelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &modelMat, sizeof(Model));
	devCon->Unmap(pModelBuffer, 0);

	devCon->Map(pCamBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &cam, sizeof(VP));
	devCon->Unmap(pCamBuffer, 0);

	// 셰이더에 바인딩
	devCon->VSSetConstantBuffers(0, 1, &pCamBuffer);
	devCon->VSSetConstantBuffers(1, 1, &pModelBuffer);

}

void init_DepthBuffer(void)
{
	ID3D11Texture2D* pDepthBuf = nullptr;
	D3D11_TEXTURE2D_DESC depthDESC = {};


	depthDESC.Width = SCREEN_SIZE_WIDTH;
	depthDESC.Height = SCREEN_SIZE_HEIGHT;
	depthDESC.MipLevels = 1;
	depthDESC.ArraySize = 1;
	depthDESC.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthDESC.SampleDesc.Count = 4;// swap chain 의 MSAA 샘플 설정과 동일하게
	depthDESC.SampleDesc.Quality = 0;

	depthDESC.Usage = D3D11_USAGE_DEFAULT;
	depthDESC.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	dev->CreateTexture2D(&depthDESC , NULL , &pDepthBuf);
	dev->CreateDepthStencilView(pDepthBuf, nullptr, &depthBuffer);

}


void CreateShaderResourceViewFromBMPFile(ID3D11Device* device, const char* fileName, UINT bmp_format ,ID3D11ShaderResourceView** Texture_SRV)
{
	//텍스처 이미지 로드
	BMPFILE textureImage = {};
	LoadBmpFile(fileName, &textureImage, bmp_format);
	BYTE* image_32bit = (BYTE*)malloc(textureImage.width * textureImage.height * 4);

	for (int i = 0; i < textureImage.height; i++)
	{
		int srcRowOffset = i * (textureImage.width * textureImage.format + textureImage.widthPadding);
		int dstRowOffset = i * textureImage.width * 4;

		for (int j = 0; j < textureImage.width; j++)
		{
			image_32bit[dstRowOffset + j * 4 + 0] = textureImage.data[srcRowOffset + j * textureImage.format + 0];
			image_32bit[dstRowOffset + j * 4 + 1] = textureImage.data[srcRowOffset + j * textureImage.format + 1];
			image_32bit[dstRowOffset + j * 4 + 2] = textureImage.data[srcRowOffset + j * textureImage.format + 2];
			image_32bit[dstRowOffset + j * 4 + 3] = (bmp_format == BMP_FORMAT_BGR) ? 255 : textureImage.data[srcRowOffset + j * textureImage.format + 3];
		}
		
	}

	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = textureImage.width;
	textureDesc.Height = textureImage.height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;

	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	//리소스 구조체 선언
	D3D11_SUBRESOURCE_DATA source_data;
	source_data.pSysMem = image_32bit;
	//BMPFILE의 format은 한셀의 바이트 수 와 동일
	source_data.SysMemPitch = textureImage.width * BMP_FORMAT_BGRA;
	source_data.SysMemSlicePitch = 0;//3D 텍스처에서의 1장의 크기 2D의 경우 0

	//texture2D 구조 생성
	ID3D11Texture2D* pTexture2D = nullptr;
	device->CreateTexture2D(&textureDesc, &source_data, &pTexture2D);

	//SRV 생성 DirectX 에서는 view단위로???
	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = textureDesc.Format;
	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = 1;

	device->CreateShaderResourceView(pTexture2D,&srv_desc,Texture_SRV);
	pTexture2D->Release();
	free(image_32bit);

}