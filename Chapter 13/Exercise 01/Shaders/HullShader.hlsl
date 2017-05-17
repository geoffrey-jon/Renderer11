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

cbuffer cbPerFrame : register(b0)
{
	float3 gEyePosW;
	float pad;
};

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
};

struct VertexOut
{
	float3 PosL    : POSITION;
};

struct PatchTess
{
	float EdgeTess[3]   : SV_TessFactor;
	float InsideTess[1] : SV_InsideTessFactor;
};

struct HullOut
{
	float3 PosL : POSITION;
};

PatchTess ConstantHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float3 centerL = (patch[0].PosL + patch[1].PosL + patch[2].PosL) / 3.0f;
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;

	float d = distance(centerW, gEyePosW);

	// Tessellate the patch based on distance from the eye such that
	// the tessellation is 0 if d >= d1 and 60 if d <= d0.  The interval
	// [d0, d1] defines the range we tessellate in.

	const float d0 = 20.0f;
	const float d1 = 100.0f;
	float tess = 64.0f*saturate((d1 - d) / (d1 - d0));

	// Uniformly tessellate the patch.

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;

	pt.InsideTess[0] = tess;

	return pt;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 3> p,
	uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	hout.PosL = p[i].PosL;

	return hout;
}