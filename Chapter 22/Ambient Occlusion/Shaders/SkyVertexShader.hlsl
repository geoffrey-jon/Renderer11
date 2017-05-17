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

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
	float3 Tangent : TANGENT;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float3 PosL    : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj).xyww;

	// Use local vertex position as cubemap lookup vector.
	vout.PosL = vin.PosL;

	return vout;
}