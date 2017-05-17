//***************************************************************************************
//***************************************************************************************

#include "LightHelper.hlsl"

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldView;
	float4x4 gWorldInvTransposeView;
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 PosL     : POSITION;
	float3 NormalL  : NORMAL;
};

struct VertexOut
{
	float4 PosH       : SV_POSITION;
	float3 PosV       : POSITION;
    float3 NormalV    : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosV = mul(float4(vin.PosL, 1.0f), gWorldView).xyz;
	vout.NormalV = mul(vin.NormalL, (float3x3)gWorldInvTransposeView);

	// Do I need this?
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
    return vout;
}
