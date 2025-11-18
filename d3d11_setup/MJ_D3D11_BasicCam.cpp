#include "MJ_D3D11_BasicCam.h"
#include <stdio.h>
using namespace DirectX;

DirectX::XMMATRIX originCam = {
	0.F, 0.F, 1.F, 1.F,
	0.F, 0.F, 0.F, 1.F,
	0.F, 1.F, 0.F, 0.F,
	0.F, 0.F, 0.F, 1.F
};




BasicCam::BasicCam()
{
	this->camElementMat = {
		0.F , 0.F , 1.F , 1.F,
		0.F , 0.F , 0.F , 1.F,
		0.F , 1.F , 0.F  , 0.F,
		0.F , 0.F , 0.F , 1.F
	};

	this->moveMat = XMMatrixIdentity();
	this->rotMat = XMMatrixIdentity();

	
}

BasicCam::~BasicCam()
{
	
}


void BasicCam::SetUpV4f(XMVECTOR up)
{
	this->Element.up = up;
}

void BasicCam::SetPosV4f(XMVECTOR pos)
{
	this->Element.pos = pos;
}

void BasicCam::SetAtV4f(XMVECTOR at)
{
	this->Element.at = at;
}

   

void BasicCam::MoveFrontBack(double speed)
{
	XMVECTOR dir = XMVectorSubtract(this->Element.at ,this->Element.pos);// 여기서 w는 0이됨

	XMVECTOR unitAt = XMVector4Normalize(dir);
	unitAt = XMVectorScale(unitAt , speed);	
	XMMATRIX moveMat = XMMatrixTranslation(unitAt.m128_f32[0],
										   unitAt.m128_f32[1],
										   unitAt.m128_f32[2]);
	this->moveMat = XMMatrixTranslation(this->Element.pos.m128_f32[0],
					this->Element.pos.m128_f32[1],
					this->Element.pos.m128_f32[2]);
	this->moveMat = XMMatrixMultiply(this->moveMat, moveMat);

	this->CamTranform();
}


void BasicCam::MoveLeftRight(double speed)
{
	XMVECTOR dir = XMVectorSubtract(this->Element.at, this->Element.pos);// 여기서 w는 0이됨

	XMVECTOR unitAt = XMVector4Normalize(dir);
	unitAt = XMVector3Cross(this->Element.up , unitAt);


	unitAt = XMVectorScale(unitAt, speed);
	XMMATRIX moveMat = XMMatrixTranslation(unitAt.m128_f32[0], 
										   unitAt.m128_f32[1], 
										   unitAt.m128_f32[2]);


	this->moveMat = XMMatrixTranslation(this->Element.pos.m128_f32[0],
					this->Element.pos.m128_f32[1],
					this->Element.pos.m128_f32[2]);

	this->moveMat = XMMatrixMultiply(this->moveMat, moveMat);

	this->CamTranform();
}




void BasicCam::RotToAt(FXMVECTOR axis, double angle)
{
	FXMVECTOR quatRotVector = XMQuaternionRotationAxis(axis, angle);
	XMMATRIX quatRotMat = XMMatrixRotationQuaternion(quatRotVector);
	this->rotMat = XMMatrixMultiply(this->rotMat, quatRotMat);
	this->moveMat = XMMatrixTranslation(this->Element.pos.m128_f32[0],
					this->Element.pos.m128_f32[1],
					this->Element.pos.m128_f32[2]);
	this->CamTranform();
}

void BasicCam::CamTranform()
{

	
	this->camElementMat = XMMatrixMultiply(originCam , this->rotMat);
	// up 벡터의 선형성을 보존하기 위해서 up 벡터의 경우 방향 벡터이므로 translate 연산이 이루어지면 안됨
	this->camElementMat.r[2] = { 0 , 1 ,0 ,0 };

	this->camElementMat = XMMatrixMultiply(this->camElementMat , this->moveMat);
	
	
}


void BasicCam::TracballRoate(DirectX::XMINT2 ScreenStart, DirectX::XMINT2 ScreenCur, int ScreenWidth, int ScreenHeight, double RotateRatio)
{
	//인풋 정규화 과정
	XMVECTOR mouseStartV2f = { ((double)ScreenStart.x / ScreenWidth) * 2.F - 1.F ,-1*((double)ScreenStart.y / ScreenHeight) * 2.F + 1.F , 0.F , 0.F };
	XMVECTOR mouseCurV2f = { ((double)ScreenCur.x / ScreenWidth) * 2.F - 1.F ,-1*((double)ScreenCur.y / ScreenHeight) * 2.F + 1.F , 0.F ,0.F };


	XMVECTOR moveVector = XMVectorSubtract(mouseCurV2f , mouseStartV2f);

	XMVECTOR theta1f = XMVector2Length(moveVector) * RotateRatio;
	XMVECTOR tempAxis = XMVector3Cross({ 0 , 0, 1.F , 0 } ,moveVector);
	XMMATRIX inverseRotMat = XMMatrixTranspose(this->rotMat);
	XMVECTOR axis = {0,0,0,0};
	axis.m128_f32[0] = XMVector4Dot(inverseRotMat.r[0] , tempAxis).m128_f32[0];
	axis.m128_f32[1] = XMVector4Dot(inverseRotMat.r[1], tempAxis).m128_f32[1];
	axis.m128_f32[2] = XMVector4Dot(inverseRotMat.r[2], tempAxis).m128_f32[2];
	axis.m128_f32[3] = XMVector4Dot(inverseRotMat.r[3], tempAxis).m128_f32[3];


	if (XMVector4Equal(axis, { 0,0,0,0.F })) return;
	
	
	this->RotToAt(axis, theta1f.m128_f32[0]);

}


void BasicCam::TracballRoateNormVector(XMVECTOR MoveVector, double RotateRatio)
{
	if (XMVector4Equal(MoveVector, { 0,0,0,0.F })) return;
	
	//인풋 정규화 과정
	XMVECTOR theta1f = XMVector4Length(MoveVector);
	XMVECTOR tempAxis = XMVector3Cross({ 0 , 0, 1.F , 0 }, MoveVector);
	XMMATRIX inverseRotMat = XMMatrixTranspose(this->rotMat);
	XMVECTOR axis = { 0,0,0,0 };
	axis.m128_f32[0] = XMVector4Dot(inverseRotMat.r[0], tempAxis).m128_f32[0];
	axis.m128_f32[1] = XMVector4Dot(inverseRotMat.r[1], tempAxis).m128_f32[1];
	axis.m128_f32[2] = XMVector4Dot(inverseRotMat.r[2], tempAxis).m128_f32[2];
	axis.m128_f32[3] = XMVector4Dot(inverseRotMat.r[3], tempAxis).m128_f32[3];
	if (XMVector4Equal(axis, { 0,0,0,0.F })) return;

	
	this->RotToAt(axis, XMVector2Length(MoveVector).m128_f32[0] * RotateRatio);

}