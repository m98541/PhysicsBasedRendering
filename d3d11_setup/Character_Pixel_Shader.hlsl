Texture2DArray txDiffuse : register(t0);
SamplerState samLinear : register(s0);

// 픽셀 쉐이더 는 변화 X -> 기존 그냥 그대로 사용 방법은 없을끼?....
struct PS_INPUT {
	float4 position : SV_POSITION;
	float2 tex_coord : TX_CRD;
	uint tex_idx : TX_IDX;
    float4 norm : NORM;
};

float4 main(PS_INPUT ps_input) : SV_TARGET
{
    float4 pixel = txDiffuse.Sample(samLinear, float3(ps_input.tex_coord, ps_input.tex_idx));
	return pixel;	
}