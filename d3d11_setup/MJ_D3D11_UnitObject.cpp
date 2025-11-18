#include "MJ_D3D11_UnitObject.h"

using namespace UnitObject;
using namespace DirectX;
using namespace CapsuleCollision;

UnitObj::UnitObj(ID3D11Device* Dev, ID3D11DeviceContext* DevCon, ID3D10Blob* vsShader, MJD3D11OBJ_HANDLE_t* objHandle, CAPSULE_T mapCollider, ConvexHull* objCollider)
{
	this->TRS = XMMatrixIdentity();
	this->scale = XMFLOAT4(1.F , 1.F , 1.F, 1.F);
	this->rotate = 0.0;
	this->pos = XMFLOAT4(0 , 0, 0, 1.F);
	this->moveActive = true;

	this->mapCollider = mapCollider;
	this->Dev = Dev;
	this->DevCon = DevCon;
	this->vsShader = vsShader;
	this->objHandle = objHandle;
	this->objCollider = objCollider;

	this->modelBuffer = nullptr;
	this->sampler = nullptr;

	this->textureSRV = nullptr;

	D3D11_BUFFER_DESC cbd = {};
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.ByteWidth = sizeof(XMMATRIX);
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Dev->CreateBuffer(&cbd, nullptr, &this->modelBuffer);
	this->LoadColliderView();
	



	
	D3D11_RASTERIZER_DESC wfDesc = {};
	wfDesc.FillMode = D3D11_FILL_WIREFRAME;  // 선만 그립니다.
	wfDesc.CullMode = D3D11_CULL_NONE;       // 모든 선을 봐야 하므로 컬링을 끕니다. (혹은 CULL_BACK)
	wfDesc.FrontCounterClockwise = false;
	wfDesc.DepthClipEnable = true;
	Dev->CreateRasterizerState(&wfDesc, &this->pWireframeRS);


	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;      // 면을 채웁니다.
	rsDesc.CullMode = D3D11_CULL_BACK;       // 뒷면을 그리지 않습니다.
	rsDesc.FrontCounterClockwise = false;
	rsDesc.DepthClipEnable = true;
	Dev->CreateRasterizerState(&rsDesc, &this->pSolidRS);


}

UnitObj::~UnitObj()
{
	free(this->colliderObjHandle);
}

void UnitObj::LoadColliderView()
{
	this->colliderObjHandle = (MJD3D11OBJ_HANDLE_t*)malloc(sizeof(MJD3D11OBJ_HANDLE_t));

	D3D11_BUFFER_DESC vertexBufferDesc = {};
	memset(&vertexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	D3D11_BUFFER_DESC indexBufferDesc = {};
	memset(&indexBufferDesc, 0, sizeof(D3D11_BUFFER_DESC));

	D3D11_INPUT_ELEMENT_DESC inputElementDesc = {};
	memset(&inputElementDesc, 0, sizeof(D3D11_INPUT_ELEMENT_DESC));
	
	this->colliderObjHandle->vertexCount = this->objCollider->vertexArray.size();
	this->colliderObjHandle->indexBufferSize = this->objCollider->indexArray.size();

	vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.ByteWidth = sizeof(VERTEX_T) * this->colliderObjHandle->vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Dev->CreateBuffer(&vertexBufferDesc, NULL, &this->colliderObjHandle->vertexBufferHandle);

	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(UINT) * this->colliderObjHandle->indexBufferSize;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;

	this->colliderObjHandle->indexBuffer = this->objCollider->indexArray.data();

	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem = this->objCollider->indexArray.data();
	Dev->CreateBuffer(&indexBufferDesc, &indexData, &this->colliderObjHandle->indexBufferHandle);

	DevCon->IASetIndexBuffer(this->colliderObjHandle->indexBufferHandle, DXGI_FORMAT_R32_UINT, 0);

	D3D11_MAPPED_SUBRESOURCE mappingSrc;
	DevCon->Map(this->colliderObjHandle->vertexBufferHandle, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &mappingSrc);

	memcpy(mappingSrc.pData, this->objCollider->vertexArray.data(), sizeof(VERTEX_T) * this->colliderObjHandle->vertexCount);
	DevCon->Unmap(this->colliderObjHandle->vertexBufferHandle, NULL);

	D3D11_INPUT_ELEMENT_DESC inputElement[4] = {
		//0바이트 부터 
		{"POSITION" , 0 , DXGI_FORMAT_R32G32B32A32_FLOAT, 0 , 0  , D3D11_INPUT_PER_VERTEX_DATA , 0},
		//12바이트 부터
		{"TEXCOORD" , 0 , DXGI_FORMAT_R32G32_FLOAT, 0 , 16 , D3D11_INPUT_PER_VERTEX_DATA , 0},

		{"NORM" , 0 , DXGI_FORMAT_R32G32B32A32_FLOAT, 0 , 24 , D3D11_INPUT_PER_VERTEX_DATA , 0},

		{"TEXIDX" , 0 ,DXGI_FORMAT_R32_UINT , 0 , 40 , D3D11_INPUT_PER_VERTEX_DATA , 0}

	};
	Dev->CreateInputLayout(inputElement, 4, this->vsShader->GetBufferPointer(), this->vsShader->GetBufferSize(), &this->colliderObjHandle->inputLayout);
	DevCon->IASetInputLayout(this->colliderObjHandle->inputLayout);

}

void UnitObj::DrawCollider()
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	DevCon->Map(this->modelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &this->TRS, sizeof(XMMATRIX));
	DevCon->Unmap(this->modelBuffer, 0);
	DevCon->VSSetConstantBuffers(1, 1, &this->modelBuffer);

	UINT stride = sizeof(VERTEX_T);
	UINT offset = 0;
	DevCon->PSSetShaderResources(0, 1, this->objHandle->textureResourceViewHandleArr);
	DevCon->PSSetSamplers(0, 1, &this->objHandle->samplerHandle);

	DevCon->IASetVertexBuffers(0, 1, &this->colliderObjHandle->vertexBufferHandle, &stride, &offset);
	DevCon->IASetIndexBuffer(this->colliderObjHandle->indexBufferHandle, DXGI_FORMAT_R32_UINT, 0);

	DevCon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DevCon->RSSetState(this->pWireframeRS);
	DevCon->DrawIndexed(this->colliderObjHandle->indexBufferSize, 0, 0);
	DevCon->RSSetState(this->pSolidRS);

}
void UnitObj::DrawObject()
{
	D3D11_MAPPED_SUBRESOURCE mapped;
	DevCon->Map(this->modelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &this->TRS, sizeof(XMMATRIX));
	DevCon->Unmap(this->modelBuffer, 0);
	DevCon->VSSetConstantBuffers(1, 1, &this->modelBuffer);
	MJD3D11DrawOBJ(this->DevCon , this->objHandle);
}



void UnitObj::updateTRS()
{
	this->TRS = XMMatrixIdentity();
	// new TRS 에 scale rot trans 곱
	this->TRS *= XMMatrixScaling(this->scale.x, this->scale.y, this->scale.z);
	this->TRS *= XMMatrixRotationY(this->rotate);
	this->TRS *= XMMatrixTranslation(this->pos.x, this->pos.y, this->pos.z);

	D3D11_MAPPED_SUBRESOURCE mapped;
	DevCon->Map(this->modelBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	memcpy(mapped.pData, &this->TRS, sizeof(XMMATRIX));
	DevCon->Unmap(this->modelBuffer, 0);
	DevCon->VSSetConstantBuffers(1, 1, &this->modelBuffer);
}

void UnitObj::setPos(XMFLOAT4 pos)
{
	if (this->moveActive)
	{
		this->pos = pos;
		this->updateTRS();
	}
	
}
void UnitObj::setRotate(double rotate)
{
	if (this->moveActive)
	{
		this->rotate = rotate;
		this->updateTRS();
	}
}
void UnitObj::setScale(XMFLOAT4 scale)
{
	if (this->moveActive)
	{
		this->scale = scale;
		this->updateTRS();
	}
}

XMFLOAT4 UnitObj::getPos()
{
	return this->pos;
}
XMFLOAT4 UnitObj::getScale()
{
	return this->scale;
}
double UnitObj::getRotate()
{
	return this->rotate;
}
XMMATRIX UnitObj::getTRS()
{
	return this->TRS;
}

void UnitObj::Stop()
{
	this->moveActive = false;
}

UNIT_BOX_T UnitObj::getBOXCollider()
{
	return this->boxCollider;
}

bool UnitObj::moveState()
{
	return this->moveActive;
}

void UnitObj::setBox()
{
	
	this->boxCollider.boxMax = XMVector3Transform(XMLoadFloat4(&this->objCollider->vertexArray[0].pos), this->TRS);
	this->boxCollider.boxMin = this->boxCollider.boxMax;

	XMVECTOR v;
	for (int i = 1; i < this->objCollider->vertexArray.size(); i++)
	{
		v = XMVector3Transform(XMVectorScale(XMLoadFloat4(&this->objCollider->vertexArray[i].pos), 1.1), this->TRS);
		
		if (v.m128_f32[0] > this->boxCollider.boxMax.m128_f32[0])
		{
			this->boxCollider.boxMax.m128_f32[0] = v.m128_f32[0];
		}

		if ( v.m128_f32[1] > this->boxCollider.boxMax.m128_f32[1])
		{
			this->boxCollider.boxMax.m128_f32[1] = v.m128_f32[1];
		}

		if (v.m128_f32[2] > this->boxCollider.boxMax.m128_f32[2])
		{
			this->boxCollider.boxMax.m128_f32[2] = v.m128_f32[2];
		}




		if (v.m128_f32[0] < this->boxCollider.boxMin.m128_f32[0])
		{
			this->boxCollider.boxMin.m128_f32[0] = v.m128_f32[0];
		}

		if (v.m128_f32[1] < this->boxCollider.boxMin.m128_f32[1])
		{
			this->boxCollider.boxMin.m128_f32[1] = v.m128_f32[1];
		}

		if (v.m128_f32[2] < this->boxCollider.boxMin.m128_f32[2])
		{
			this->boxCollider.boxMin.m128_f32[2] = v.m128_f32[2];
		}
	}

}

bool UnitObj::unitAABBCollCheck(UnitObj* other)
{

	UNIT_BOX_T boxA = this->getBOXCollider();
	UNIT_BOX_T boxB = other->getBOXCollider();
	// 한 축이라도 분리되어 있으면 충돌하지 않음
	if (boxA.boxMax.m128_f32[0] <= boxB.boxMin.m128_f32[0] || // A가 B의 왼쪽
		boxA.boxMin.m128_f32[0] >= boxB.boxMax.m128_f32[0] || // A가 B의 오른쪽
		boxA.boxMax.m128_f32[1] <= boxB.boxMin.m128_f32[1] || // A가 B의 아래
		boxA.boxMin.m128_f32[1] >= boxB.boxMax.m128_f32[1] || // A가 B의 위
		boxA.boxMax.m128_f32[2] <= boxB.boxMin.m128_f32[2] || // A가 B의 뒤
		boxA.boxMin.m128_f32[2] >= boxB.boxMax.m128_f32[2])   // A가 B의 앞
	{
		return true; // 분리됨 (충돌 아님)
	}

	return false; // 분리되지 않음 (충돌)
}