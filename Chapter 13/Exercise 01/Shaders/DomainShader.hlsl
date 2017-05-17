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

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
};

struct PatchTess
{
	float EdgeTess[3]   : SV_TessFactor;
	float InsideTess[1] : SV_InsideTessFactor;
};

struct DomainOut
{
	float4 PosH : SV_POSITION;
};

struct HullOut
{
	float3 PosL : POSITION;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("tri")]
DomainOut DS(PatchTess patchTess,
	float3 uvw : SV_DomainLocation,
	const OutputPatch<HullOut, 3> tri)
{
	DomainOut dout;

	// Bilinear interpolation.
//	float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
//	float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
//	float3 p = lerp(v1, v2, uv.y);
	float3 p = (uvw.x*tri[0].PosL) + (uvw.y*tri[1].PosL) + (uvw.z*tri[2].PosL);

	// Displacement mapping
	p.y = 0.1f*(p.z*sin(p.x) + p.x*cos(p.z));

	dout.PosH = mul(float4(p, 1.0f), gWorldViewProj);

	return dout;
}