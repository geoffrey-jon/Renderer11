//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

//***************************************************************************************
// LightHelper.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Structures and functions for lighting calculations.
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
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

cbuffer cbPSParams : register(b2)
{
	int gUseTexure;
	int gAlphaClip;
	int gFogEnabled;
	int gReflectionEnabled;
	int gUseNormal;
	int3 pad2;
};

Texture2D gDiffuseMap;
Texture2D gNormalMap;
TextureCube gCubeMap;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

struct VertexOut
{
	float4 PosH     : SV_POSITION;
	float3 PosW     : POSITION;
	float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex      : TEXCOORD;
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

	// Default to multiplicative identity.
	float4 texColor = float4(1, 1, 1, 1);
	if (gUseTexure > 0)
	{
		// Sample texture.
		texColor = gDiffuseMap.Sample(samLinear, pin.Tex);

		if (gAlphaClip > 0)
		{
			// Discard pixel if texture alpha < 0.1.  Note that we do this
			// test as soon as possible so that we can potentially exit the shader 
			// early, thereby skipping the rest of the shader code.
			clip(texColor.a - 0.1f);
		}
	}

	//
	// Normal mapping
	//
	float3 bumpedNormalW = float3(0, 0, 0);
	if (gUseNormal > 0)
	{
		float3 normalMapSample = gNormalMap.Sample(samLinear, pin.Tex).rgb;
		bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);
	}

	//
	// Lighting.
	//

	float4 litColor = texColor;
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
			if (gUseNormal)
			{
				ComputeDirectionalLight(gMaterial, gDirLights[i], bumpedNormalW, toEye, A, D, S);
			}
			else
			{
				ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, A, D, S);
			}

			ambient += A;
			diffuse += D;
			spec += S;
		}

		// Modulate with late add.
		litColor = texColor*(ambient + diffuse) + spec;

		if (gReflectionEnabled > 0)
		{
			float3 incident = -toEye;
			float3 reflectionVector = reflect(incident, pin.NormalW);
			float4 reflectionColor = gCubeMap.Sample(samLinear, reflectionVector);

			litColor += gMaterial.Reflect*reflectionColor;
		}
	}

	// Common to take alpha from diffuse material and texture.
	litColor.a = gMaterial.Diffuse.a * texColor.a;

	return litColor; 
}