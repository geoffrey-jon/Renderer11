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
    float gMinDist;
    float gMaxDist;
    float gMinTess;
    float gMaxTess;
    float gTexelCellSpaceU;
    float gTexelCellSpaceV;
    float gWorldCellSpace;
};

cbuffer cbPerObject : register(b1)
{
    float4x4 gViewProj;
    Material gMaterial;
};

struct DomainOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 TiledTex : TEXCOORD1;
};

Texture2D gHeightMap;
Texture2DArray gLayerMapArray;
Texture2D gBlendMap;

SamplerState samHeightmap : register(s0)
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

SamplerState samLinear : register(s1)
{
    Filter = MIN_MAG_MIP_LINEAR;

    AddressU = WRAP;
    AddressV = WRAP;
};

float4 PS(DomainOut pin) : SV_Target
{
    int gLightCount = 3;
    bool gFogEnabled = false;

	//
	// Estimate normal and tangent using central differences.
	//
    float2 leftTex = pin.Tex + float2(-gTexelCellSpaceU, 0.0f);
    float2 rightTex = pin.Tex + float2(gTexelCellSpaceU, 0.0f);
    float2 bottomTex = pin.Tex + float2(0.0f, gTexelCellSpaceV);
    float2 topTex = pin.Tex + float2(0.0f, -gTexelCellSpaceV);
	
    float leftY = gHeightMap.SampleLevel(samHeightmap, leftTex, 0).r;
    float rightY = gHeightMap.SampleLevel(samHeightmap, rightTex, 0).r;
    float bottomY = gHeightMap.SampleLevel(samHeightmap, bottomTex, 0).r;
    float topY = gHeightMap.SampleLevel(samHeightmap, topTex, 0).r;
	
    float3 tangent = normalize(float3(2.0f * gWorldCellSpace, rightY - leftY, 0.0f));
    float3 bitan = normalize(float3(0.0f, bottomY - topY, -2.0f * gWorldCellSpace));
    float3 normalW = cross(tangent, bitan);
//    float3 normalW = float3(0.0f, 1.0f, 0.0f);


	// The toEye vector is used in lighting.
    float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
    float distToEye = length(toEye);

	// Normalize.
    toEye /= distToEye;
	
	//
	// Texturing
	//
	
	// Sample layers in texture array.
    float4 c0 = gLayerMapArray.Sample(samLinear, float3(pin.TiledTex, 0.0f));
    float4 c1 = gLayerMapArray.Sample(samLinear, float3(pin.TiledTex, 1.0f));
    float4 c2 = gLayerMapArray.Sample(samLinear, float3(pin.TiledTex, 2.0f));
    float4 c3 = gLayerMapArray.Sample(samLinear, float3(pin.TiledTex, 3.0f));
    float4 c4 = gLayerMapArray.Sample(samLinear, float3(pin.TiledTex, 4.0f));
	
	// Sample the blend map.
    float4 t = gBlendMap.Sample(samLinear, pin.Tex);
    
    // Blend the layers on top of each other.
    float4 texColor = c0;
    texColor = lerp(texColor, c1, t.r);
    texColor = lerp(texColor, c2, t.g);
    texColor = lerp(texColor, c3, t.b);
    texColor = lerp(texColor, c4, t.a);
 
	//
	// Lighting.
	//

//    float4 texColor = float4(1.0f, 0.0f, 0.0f, 1.0f);

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
            ComputeDirectionalLight(gMaterial, gDirLights[i], normalW, toEye,
				A, D, S);

            ambient += A;
            diffuse += D;
            spec += S;
        }

        litColor = texColor * (ambient + diffuse) + spec;
    }
 
	//
	// Fogging
	//

//    if (gFogEnabled)
//    {
//        float fogLerp = saturate((distToEye - gFogStart) / gFogRange);

		// Blend the fog color and the lit color.
//        litColor = lerp(litColor, gFogColor, fogLerp);
//    }

    return litColor;
}