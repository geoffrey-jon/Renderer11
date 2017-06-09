//=============================================================================
//=============================================================================

#include "LightHelper.hlsl"

#define PT_EMITTER 0
#define PT_FLARE 1

cbuffer cbPerFrame : register(b0)
{
    DirectionalLight gLights[3];
    float4x4 gViewProj;
    Material gMaterial;
    float3 gEyePosW;
    float pad;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

struct GeoOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 Tex   : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

[maxvertexcount(4)]
void GS(point VertexOut gin[1], uint primId : SV_PrimitiveID, inout TriangleStream<GeoOut> triStream)
{
	// Compute world matrix so that billboard faces the camera.
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW.xyz - gin[0].CenterW;
    look.y = 0.0f;
    look = normalize(look);
    float3 right = cross(up, look);
		
	// Compute triangle strip vertices (quad) in world space.
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
	
    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);
		
	// Transform quad vertices to world space and output them as a triangle strip.
    GeoOut gout;

    float2 gQuadTexC[4] =
    {
        float2(0.0f, 1.0f),
    	float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
	    float2(1.0f, 0.0f)
    };

	[unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.Tex = gQuadTexC[i];
        gout.PrimID = primId;

        triStream.Append(gout);
    }
}
