//***************************************************************************************
//***************************************************************************************

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEXCOORD;
};

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

Texture2D gTexture;

float4 PS(VertexOut pin) : SV_Target
{
    float4 c = gTexture.Sample(samLinear, pin.Tex);
	
	// draw as grayscale
    float g = c[0];
    return float4(g.rrr, 1);
}