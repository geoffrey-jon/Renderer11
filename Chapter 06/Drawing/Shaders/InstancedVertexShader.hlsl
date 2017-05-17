//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject
{
	float4x4 gViewProj;
};

struct VertexIn
{
	float3 Pos   : POSITION;
	float4 Color : COLOR;
	float4 TexCoord0 : TEXCOORD0;
	float4 TexCoord1 : TEXCOORD1;
	float4 TexCoord2 : TEXCOORD2;
	float4 TexCoord3 : TEXCOORD3;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	float4x4 world = float4x4(vin.TexCoord0, vin.TexCoord1, vin.TexCoord2, vin.TexCoord3);
	float4x4 worldViewProj = mul(world, gViewProj);

	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.Pos, 1.0f), worldViewProj);

	// Just pass vertex color into the pixel shader.
	vout.Color = vin.Color;

	return vout;
}
