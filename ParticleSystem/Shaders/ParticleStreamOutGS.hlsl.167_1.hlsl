//=============================================================================
// Fire.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Fire particle system.  Particles are emitted directly in world space.
//=============================================================================

#define PT_EMITTER 0
#define PT_FLARE 1

cbuffer cbPerFrameParticle : register(b0)
{
    float4 gEyePosW;
    float4 gEmitPosW;
    float4 gEmitDirW;
	
    float gTimeStep;
    float gGameTime;
    float2 pad2;
    float4x4 gViewProj;
};

struct Particle
{
    float3 InitialPosW : POSITION;
    float3 InitialVelW : VELOCITY;
    float2 SizeW : SIZE;
    float Age : AGE;
    uint Type : TYPE;
};

// Random texture used to generate random numbers in shaders.
Texture1D gRandomTex;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

float3 RandUnitVec3(float offset)
{
	// Use game time plus offset to sample random texture.
    float u = (gGameTime + offset);
	
	// coordinates in [-1,1]
    float3 v = gRandomTex.SampleLevel(samLinear, u, 0).xyz;
	
	// project onto unit sphere
    return normalize(v);
}

[maxvertexcount(2)]
void GS(point Particle gin[1], inout PointStream<Particle> ptStream)
{
    gin[0].Age += gTimeStep;
	
    if (gin[0].Type == 0)
    {
		// time to emit a new particle?
        if (gin[0].Age > 0.005f)
        {
            float3 vRandom = RandUnitVec3(0.0f);
            vRandom.x *= 0.5f;
            vRandom.z *= 0.5f;
			
            Particle p;
            p.InitialPosW = gEmitPosW.xyz;
            p.InitialVelW = 4.0f * vRandom;
            p.SizeW = float2(3.0f, 3.0f);
            p.Age = 0.0f;
            p.Type = 1;
			
            ptStream.Append(p);
			
			// reset the time to emit
            gin[0].Age = 0.0f;
        }
		
		// always keep emitters
        ptStream.Append(gin[0]);
    }
    else
    {
		// Specify conditions to keep particle; this may vary from system to system.
        if (gin[0].Age <= 1.0f)
            ptStream.Append(gin[0]);
    }
}