#ifndef MJ_D3D11_CHARACTER_RENDERER_H
#define MJ_D3D11_CHARACTER_RENDERER_H
// 해당 객체에서 d3d11 쉐이더의 관리 부분에 대한 책임을 짐 character 객체로 부터 렌더링에 필요한 데이터 받음
#define NOMINMAX
#include "MJ_D3D11_Character.h"
#include "MJ_D3D11_ShaderFile.h"


class CharacterRenderer
{
	public:
		CharacterRenderer(ID3D11Device* dev, ID3D11DeviceContext* devCon, Character& character);
		CharacterRenderer();
	
		~CharacterRenderer();

		void SetPipeLine(ID3D11DeviceContext* devCon);
		void SetCharacter	(ID3D11DeviceContext* devCon, Character& character);

		void InitPipeLine(
			ID3D11Device* dev,
			ID3D11DeviceContext* devCon,
			ShaderFile& vsFile,
			ShaderFile& psFile,
			D3D11_INPUT_ELEMENT_DESC* inputElement,
			int inputElementCount);

		void Draw(ID3D11DeviceContext* devCon);

		// 캐릭터의 상수버퍼 업데이트 즉 캐릭터가 이미 가진 정보 -> gpu 에 업데이트 
		// 현재는 키프레임 방식인 아닌 변화된 행렬을 연속적으로 업데이트 해주는 방식으로 매프레임 발생 해야함
		// 위 매프레임 업데이트가 정공하면 키프레임 방식을 도입하여 업데이트 주기를 늘려 오버헤드 해소 필요함

		void CBModelUpdate(ID3D11DeviceContext* devCon, DirectX::XMMATRIX& modelMat);
		void CBCamUpdate(ID3D11DeviceContext* devCon, DirectX::XMMATRIX& viewMat, DirectX::XMMATRIX& projMat);
		void CBJointsUpdate(ID3D11DeviceContext* devCon, DirectX::XMMATRIX* jointsPoseMatArr,uint32_t activeJointsCount);

	private:
		
		void InitData(ID3D11Device* dev, ID3D11DeviceContext* devCon ,Character& character);

		void CBInit(ID3D11Device* dev);
		
		void CBModelNDCUpdate(ID3D11DeviceContext* devCon, Character& character);

		ID3D11VertexShader* characterVertexShader;
		ID3D11PixelShader* characterPixelShader;
		ID3D11InputLayout* inputLayout;
		ID3D11Buffer* verticesBufferHandle;
		ID3D11Buffer* indicesBufferHandle;
		uint32_t indicesBufferSize;
		ID3D11ShaderResourceView** textureSRVHandleArr;
		ID3D11SamplerState* samplerHandle;

		ID3D11Buffer* pCamBuffer;
		ID3D11Buffer* pModelBuffer;
		ID3D11Buffer* pJointsBuffer;
		ID3D11Buffer* pModelNDCBuffer;

		DirectX::XMMATRIX* jointsMatrixBuffer;

		uint32_t jointsCount;
};


#endif // !MJ_D3D11_CHARACTER_RENDERER_H
