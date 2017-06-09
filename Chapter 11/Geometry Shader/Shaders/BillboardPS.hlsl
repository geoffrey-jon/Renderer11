//=============================================================================
//=============================================================================

#include "LightHelper.hlsl"

cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gLights[3];
    float4x4 gViewProj;
    Material gMaterial;
    float3 gEyePosW;
    float pad;
};

struct GeoOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

// Array of textures for texturing the particles.
Texture2D gTexArray;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

float4 PS(GeoOut pin) : SV_TARGET
{
    int gLightCount = 3;
    bool gUseTexure = true;
    bool gAlphaClip = true;

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
    if (gUseTexure)
    {
		// Sample texture.
        texColor = gTexArray.Sample(samLinear, pin.Tex);

        if (gAlphaClip)
        {
			// Discard pixel if texture alpha < 0.1.  Note that we do this
			// test as soon as possible so that we can potentially exit the shader 
			// early, thereby skipping the rest of the shader code.
            clip(texColor.a - 0.05f);
        }
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
            ComputeDirectionalLight(gMaterial, gLights[i], pin.NormalW, toEye,
				A, D, S);

            ambient += A;
            diffuse += D;
            spec += S;
        }

		// Modulate with late add.
        litColor = texColor * (ambient + diffuse) + spec;
    }

	// Common to take alpha from diffuse material and texture.
    litColor.a = gMaterial.Diffuse.a * texColor.a;

    return litColor;
}