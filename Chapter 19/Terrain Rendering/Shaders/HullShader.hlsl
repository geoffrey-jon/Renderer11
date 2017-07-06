//***************************************************************************************
//***************************************************************************************

#include "LightHelper.hlsl"

cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gDirLights[3];
    float3 gEyePosW;
    float gMinDist;
    float gMaxDist;
    float gMinTess;
    float gMaxTess;
    float gTexelCellSpaceU;
    float gTexelCellSpaceV;
    float gWorldCellSpace;
};

struct VertexOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

struct PatchTess
{
    float EdgeTess[4]   : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosW : POSITION;
    float2 Tex  : TEXCOORD0;
};

float CalcTessFactor(float3 p)
{
    float d = distance(p, gEyePosW);

	// max norm in xz plane (useful to see detail levels from a bird's eye).
	//float d = max( abs(p.x-gEyePosW.x), abs(p.z-gEyePosW.z) );
	
    float s = saturate((d - gMinDist) / (gMaxDist - gMinDist));
	
    return pow(2, (round(lerp(gMaxTess, gMinTess, s))));
}

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	// TODO: Add Frustum Culling

	// It is important to do the tess factor calculation based on the
	// edge properties so that edges shared by more than one patch will
	// have the same tessellation factor.  Otherwise, gaps can appear.
		
	// Compute midpoint on edges, and patch center
    float3 e0 = 0.5f * (patch[0].PosW + patch[2].PosW);
    float3 e1 = 0.5f * (patch[0].PosW + patch[1].PosW);
    float3 e2 = 0.5f * (patch[1].PosW + patch[3].PosW);
    float3 e3 = 0.5f * (patch[2].PosW + patch[3].PosW);
    float3 c = 0.25f * (patch[0].PosW + patch[1].PosW + patch[2].PosW + patch[3].PosW);
		
    pt.EdgeTess[0] = CalcTessFactor(e0);
    pt.EdgeTess[1] = CalcTessFactor(e1);
    pt.EdgeTess[2] = CalcTessFactor(e2);
    pt.EdgeTess[3] = CalcTessFactor(e3);
		
    pt.InsideTess[0] = CalcTessFactor(c);
    pt.InsideTess[1] = pt.InsideTess[0];
	
    return pt;
}

[domain("quad")]
[partitioning("fractional_even")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
    HullOut hout;
	
	// Pass through shader.
    hout.PosW = p[i].PosW;
    hout.Tex = p[i].Tex;
	
    return hout;
}