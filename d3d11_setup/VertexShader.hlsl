cbuffer camBuffer : register(b0)
{
	matrix view;
	matrix proj;
};

cbuffer modelBuffer : register(b1)
{
	matrix model;
};

struct PS_INPUT {
	float4 position : SV_POSITION;
	float2 tex_coord : TX_CRD;
	uint tex_idx : TX_IDX;
	float4 normal : NORM;
};

PS_INPUT main(float4 pos : POSITION, float2 tex_coord : TEXCOORD, float4 norm : NORM, uint texidx : TEXIDX)
{
	PS_INPUT outData;
		
	outData.position = mul( proj , mul(view ,mul(model, pos)));
	outData.tex_coord = tex_coord;
	outData.tex_idx = texidx;
	return outData;
}