//=============================================================================
// Fire.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Fire particle system.  Particles are emitted directly in world space.
//=============================================================================

#define PT_EMITTER 0
#define PT_FLARE 1

cbuffer cbPerFrame : register(b0)
{
    float4 gEyePosW;
    float4 gEmitPosW;
    float4 gEmitDirW;
	
    float gTimeStep;
    float gGameTime;
    float2 pad2;

    float4x4 gViewProj;
};

struct VertexOut
{
    float3 PosW  : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type    : TYPE;
}; 

struct GeoOut
{
    float4 PosH  : SV_Position;
    float4 Color : COLOR;
    float2 Tex   : TEXCOORD;
};

[maxvertexcount(4)]
void GS(point VertexOut gin[1], inout TriangleStream<GeoOut> triStream)
{
	// do not draw emitter particles.
    if (gin[0].Type != 0)
    {

		// Compute world matrix so that billboard faces the camera.
        float3 look = normalize(gEyePosW.xyz - gin[0].PosW);
        float3 right = normalize(cross(float3(0, 1, 0), look));
        float3 up = cross(look, right);
		
		// Compute triangle strip vertices (quad) in world space.
        float halfWidth = 0.5f * gin[0].SizeW.x;
        float halfHeight = 0.5f * gin[0].SizeW.y;
	
        float4 v[4];
        v[0] = float4(gin[0].PosW + halfWidth * right - halfHeight * up, 1.0f);
        v[1] = float4(gin[0].PosW + halfWidth * right + halfHeight * up, 1.0f);
        v[2] = float4(gin[0].PosW - halfWidth * right - halfHeight * up, 1.0f);
        v[3] = float4(gin[0].PosW - halfWidth * right + halfHeight * up, 1.0f);
		
		// Transform quad vertices to world space and output them as a triangle strip.
        GeoOut gout;

        float2 gQuadTexC[4] =
        {
            float2(0.0f, 1.0f),
		    float2(1.0f, 1.0f),
    		float2(0.0f, 0.0f),
	    	float2(1.0f, 0.0f)
        };

		[unroll]
        for (int i = 0; i < 4; ++i)
        {
            gout.PosH = mul(v[i], gViewProj);
            gout.Tex = gQuadTexC[i];
            gout.Color = gin[0].Color;
            triStream.Append(gout);
        }
    }
}
