//***************************************************************************************
//***************************************************************************************

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
    float2 Tex     : TEXCOORD;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 Tex  : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Position is already in NDC Space
	vout.PosH = float4(vin.PosL, 1.0f);

	// Pass this value on to the PS
	vout.Tex = vin.Tex;

	return vout;
}