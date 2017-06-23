//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b1)
{
	float4x4 gViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
};

struct VertexIn
{
	float3 PosL      : POSITION;
	float3 NormalL   : NORMAL;
	float2 Tex       : TEXCOORD;
	row_major float4x4 World   : WORLD;
	float4 Color     : COLOR;
	uint InstanceID  : SV_InstanceID;
};

struct VertexOut
{
	float4 PosH       : SV_POSITION;
	float3 PosW       : POSITION;
    float3 NormalW    : NORMAL;
    float4 Color      : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

	// Transform to world space space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), vin.World).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3) vin.World);
		
	// Transform to homogeneous clip space.
    vout.PosH = mul(float4(vout.PosW, 1.0f), gViewProj);
	
	// Output vertex attributes for interpolation across triangle.
    vout.Color = vin.Color;

    return vout;
}
