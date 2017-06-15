//***************************************************************************************
//***************************************************************************************

#include "LightHelper.hlsl"

cbuffer cbPerFrame : register(b0)
{
	DirectionalLight gDirLights[3];
	float3 gEyePosW;
	float pad;
};

cbuffer cbPerObject : register(b1)
{
    float4x4 gViewProj;
    float4x4 gTexTransform;
    Material gMaterial;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 Color : COLOR;
};

float4 PS(VertexOut pin) : SV_Target
{
	int gLightCount = 3;

	// Interpolating normal can unnormalize it, so normalize it.
	pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
	float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
	float distToEye = length(toEye);

	// Normalize.
	toEye /= distToEye;

	//
	// Lighting.
	//
    
	float4 litColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	if (gLightCount > 0)
	{
		// Start with a sum of zero. 
		float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
		float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

		// Sum the light contribution from each light source.  
		[unroll]
		for (int i = 0; i < gLightCount; ++i)
		{
			float4 A, D, S;
			ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, A, D, S);

			ambient += A * pin.Color;
			diffuse += D * pin.Color;
			spec += S;
		}

		// Modulate with late add.
		litColor = ambient + diffuse + spec;
	}

	// Common to take alpha from diffuse material and texture.
    litColor.a = gMaterial.Diffuse.a;

	return litColor; 
}