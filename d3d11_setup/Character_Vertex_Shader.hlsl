#define MAX_JOINTS_SIZE 64

cbuffer camBuffer : register(b0)
{
	matrix view;
	matrix proj;
};

cbuffer modelBuffer : register(b1)
{
	matrix model;
};


cbuffer jointBuffer : register(b2)
{
	matrix joints[MAX_JOINTS_SIZE];
};


struct PS_INPUT {
	float4 position : SV_POSITION;
	float2 tex_coord : TX_CRD;
	uint tex_idx : TX_IDX;
	float4 normal : NORM;
};

PS_INPUT main( float4 pos : POSITION, float2 texCoord : TEXCOORD, float4 norm : NORM, uint texIdx : TEXIDX , uint jointIdx : JOINTIDX)
{
	PS_INPUT outData;
		
	// joint 연산 추가
	
	outData.position = pos;// mul( proj , mul(view ,mul(model, mul( joints[jointIdx] , pos ))));
	outData.tex_coord = texCoord;
	outData.tex_idx = texIdx;
	return outData;
}