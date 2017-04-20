//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "LightHelper.hlsl"

struct VertexIn
{
	float3 PosL     : POSITION;
	float3 NormalL  : NORMAL;
	float2 Tex      : TEXCOORD;
	float3 TangentL : TANGENT;
};

struct VertexOut
{
	float3 PosW       : POSITION;
	float3 NormalW    : NORMAL;
	float3 TangentW   : TANGENT;
	float2 Tex        : TEXCOORD;
	float  TessFactor : TESS;
};

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

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to world space space.
	vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
	vout.TangentW = mul(vin.TangentL, (float3x3)gWorld);

	// Output vertex attributes for interpolation across triangle.
	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

	float d = distance(vout.PosW, gEyePosW);

	// Normalized tessellation factor. 
	// The tessellation is 
	//   0 if d >= gMinTessDistance and
	//   1 if d <= gMaxTessDistance.  
	float tess = saturate((gMinTessDistance - d) / (gMinTessDistance - gMaxTessDistance));

	// Rescale [0,1] --> [gMinTessFactor, gMaxTessFactor].
	vout.TessFactor = gMinTessFactor + tess*(gMaxTessFactor - gMinTessFactor);

	return vout;
}
