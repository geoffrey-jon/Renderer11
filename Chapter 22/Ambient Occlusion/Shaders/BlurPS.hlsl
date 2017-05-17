//***************************************************************************************
//***************************************************************************************

cbuffer cbBlurDirection : register(b0)
{
    float gTexelWidth;
    float gTexelHeight;
    float2 pad;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float2 Tex  : TEXCOORD0;
};

Texture2D gNormalDepthMap;
Texture2D gInputMap;

static const float gWeights[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };
static const int gBlurRadius = 5;

SamplerState samLinearClamp
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;

	AddressU = CLAMP;
	AddressV = CLAMP;
};

float PS(VertexOut pin) : SV_Target
{
    float2 texOffset = float2(gTexelWidth, gTexelHeight);

    float color = gWeights[5] * gInputMap.SampleLevel(samLinearClamp, pin.Tex, 0.0f).x;
    float totalWeight = gWeights[5];

    float4 centerNormalDepth = gNormalDepthMap.SampleLevel(samLinearClamp, pin.Tex, 0.0f);

    for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        if (i == 0)
        {
            continue;
        }

        float2 tex = pin.Tex + i*texOffset;
        float4 neighborNormalDepth = gNormalDepthMap.SampleLevel(samLinearClamp, tex, 0.0f);

        if ( (dot(neighborNormalDepth.xyz, centerNormalDepth.xyz) >= 0.8f) &&
             (abs(neighborNormalDepth.a - centerNormalDepth.a) <= 0.2f) )
            {
                float weight = gWeights[i + gBlurRadius];
                color += weight * gInputMap.SampleLevel(samLinearClamp, tex, 0.0f).x;
                totalWeight += weight;
        }
    }

    color = color / totalWeight;
    return color;
}