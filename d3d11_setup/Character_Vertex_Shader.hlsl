#define MAX_JOINTS_SIZE 128

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

cbuffer modelNDCBuffer : register(b3)
{
    matrix modelNDC;
}


struct PS_INPUT {
	float4 position : SV_POSITION;
	float2 tex_coord : TX_CRD;
	uint tex_idx : TX_IDX;
    float4 norm : NORM;
};

PS_INPUT main( 
	float4 pos : POSITION,
	float4 norm : NORM,
	float2 texCoord : TEXCOORD,
	uint texIdx : TEXIDX , 
	uint4 jointsIdx : JOINTS ,
	float4 weights : WEIGHTS 
	)
{
	PS_INPUT outData;
		
	// joint 연산 추가

	matrix skinMatrix = weights[0] * joints[jointsIdx[0]] +
                        weights[1] * joints[jointsIdx[1]] +
                        weights[2] * joints[jointsIdx[2]] +
                        weights[3] * joints[jointsIdx[3]];
	
    float4 skinnedPos = mul(pos ,  skinMatrix );

	// 아직 노말 고려 X 광원 도입 계획 X

    outData.position = mul(proj, mul(view, mul(model, mul(modelNDC, skinnedPos))));
	outData.tex_coord = texCoord;
	outData.tex_idx = texIdx;
    outData.norm = norm;
	return outData;
}