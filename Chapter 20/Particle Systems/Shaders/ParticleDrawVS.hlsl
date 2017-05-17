//=============================================================================
// Fire.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Fire particle system.  Particles are emitted directly in world space.
//=============================================================================

struct Particle
{
    float3 InitialPosW : POSITION;
    float3 InitialVelW : VELOCITY;
    float2 SizeW       : SIZE;
    float Age          : AGE;
    uint Type          : TYPE;
};

struct VertexOut
{
    float3 PosW  : POSITION;
    float2 SizeW : SIZE;
    float4 Color : COLOR;
    uint Type    : TYPE;
};

VertexOut VS(Particle vin)
{
    VertexOut vout;
	
    float t = vin.Age;
    float3 gAccelW = { 0.0f, 7.8f, 0.0f };

	// constant acceleration equation
    vout.PosW = 0.5f*t*t*gAccelW + t*vin.InitialVelW + vin.InitialPosW;
	
	// fade color with time
    float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / 1.0f);
    vout.Color = float4(1.0f, 1.0f, 1.0f, opacity);
	
    vout.SizeW = vin.SizeW;
    vout.Type = vin.Type;
	
    return vout;
}
