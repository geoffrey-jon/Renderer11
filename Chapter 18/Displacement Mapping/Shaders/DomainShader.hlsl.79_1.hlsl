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

	float gHeightScale;
	float gMaxTessDistance;
	float gMinTessDistance;
	float gMinTessFactor;
	float gMaxTessFactor;
};

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gViewProj;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

struct PatchTess
{
	float EdgeTess[3]   : SV_TessFactor;
	float InsideTess    : SV_InsideTessFactor;
};

struct DomainOut
{
	float4 PosH     : SV_POSITION;
	float3 PosW     : POSITION;
	float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex      : TEXCOORD;
};

struct HullOut
{
	float3 PosW     : POSITION;
	float3 NormalW  : NORMAL;
	float3 TangentW : TANGENT;
	float2 Tex      : TEXCOORD;
};

cbuffer cbPSParams : register(b2)
{
	int gUseTexure;
	int gAlphaClip;
	int gFogEnabled;
	int gReflectionEnabled;
	int gUseNormal;
	int3 pad;
};

Texture2D gNormalMap;

SamplerState samLinear
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("tri")]
DomainOut DS(PatchTess patchTess,
	float3 bary : SV_DomainLocation,
	const OutputPatch<HullOut, 3> tri)
{
	DomainOut dout;

	// Interpolate patch attributes to generated vertices.
	dout.PosW = bary.x*tri[0].PosW + bary.y*tri[1].PosW + bary.z*tri[2].PosW;
	dout.NormalW = bary.x*tri[0].NormalW + bary.y*tri[1].NormalW + bary.z*tri[2].NormalW;
	dout.TangentW = bary.x*tri[0].TangentW + bary.y*tri[1].TangentW + bary.z*tri[2].TangentW;
	dout.Tex = bary.x*tri[0].Tex + bary.y*tri[1].Tex + bary.z*tri[2].Tex;

	// Interpolating normal can unnormalize it, so normalize it.
	dout.NormalW = normalize(dout.NormalW);

	//
	// Displacement mapping.
	//

	if (gUseNormal > 0)
	{
		// Choose the mipmap level based on distance to the eye; specifically, choose
		// the next miplevel every MipInterval units, and clamp the miplevel in [0,6].
		const float MipInterval = 20.0f;
		float mipLevel = clamp((distance(dout.PosW, gEyePosW) - MipInterval) / MipInterval, 0.0f, 6.0f);

		// Sample height map (stored in alpha channel).
		float h = gNormalMap.SampleLevel(samLinear, dout.Tex, mipLevel).a;

		// Offset vertex along normal.
		dout.PosW += (gHeightScale*(h - 1.0))*dout.NormalW;
	}

	// Project to homogeneous clip space.
	dout.PosH = mul(float4(dout.PosW, 1.0f), gViewProj);

	return dout;
}
