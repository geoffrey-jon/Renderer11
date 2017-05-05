//***************************************************************************************
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
    float4x4 gViewToTexSpaceTransform;
	float4 gFrustumFarCorners[4];
	float4 gOffsets[14];
};

struct VertexIn
{
	float3 PosL            : POSITION;
	float3 ToFarPlaneIndex : NORMAL;
    float2 Tex : TEXCOORD;
};

struct VertexOut
{
	float4 PosH       : SV_POSITION;
	float3 ToFarPlane : TEXCOORD0;
	float2 Tex        : TEXCOORD1;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Position is already in NDC Space
	vout.PosH = float4(vin.PosL, 1.0f);

    vout.ToFarPlane = gFrustumFarCorners[vin.ToFarPlaneIndex.x].xyz;

	// Pass this value on to the PS
	vout.Tex = vin.Tex;

	return vout;
}