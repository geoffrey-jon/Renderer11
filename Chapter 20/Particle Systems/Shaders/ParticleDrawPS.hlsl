//=============================================================================
// Fire.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Fire particle system.  Particles are emitted directly in world space.
//=============================================================================

struct GeoOut
{
    float4 PosH  : SV_Position;
    float4 Color : COLOR;
    float2 Tex   : TEXCOORD;
};

// Array of textures for texturing the particles.
Texture2D gTexArray;

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

float4 PS(GeoOut pin) : SV_TARGET
{
    return gTexArray.Sample(samLinear, pin.Tex, 0) * pin.Color;
}