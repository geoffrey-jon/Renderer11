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

Particle VS(Particle vin)
{
    return vin;
}
