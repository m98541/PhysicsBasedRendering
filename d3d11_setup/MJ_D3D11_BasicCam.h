#ifndef BASICCAM_H
#define BASICCAM_H
#include <DirectXMath.h>

//FXMVECTOR는 함수 인자에서만 사용하고, 변수 선언이나 대입에는 XMVECTOR를 사용해야 합니다.
class BasicCam
{
public:

	union
	{
		struct CamElement {
			DirectX::XMVECTOR at;
			DirectX::XMVECTOR pos;
			DirectX::XMVECTOR up;
		}Element;

		DirectX::XMMATRIX camElementMat;
	};
	
	DirectX::XMMATRIX moveMat;
	DirectX::XMMATRIX rotMat;

	BasicCam();
	~BasicCam();

	void SetUpV4f(DirectX::XMVECTOR up);
	void SetPosV4f(DirectX::XMVECTOR pos);
	void SetAtV4f(DirectX::XMVECTOR at);


	void TracballRoate(DirectX::XMINT2 ScreenStart, DirectX::XMINT2 ScreenCur, int ScreenWidth, int ScreenHeight, float RotateRatio);
	void TracballRoateNormVector(DirectX::XMVECTOR MoveVector, float RotateRatio);

	void MoveFrontBack(float speed);
	void MoveLeftRight(float speed);
	void RotToAt(DirectX::FXMVECTOR axis, float angle);
	void CamTranform();

};





#endif // !BASICCAM_H
