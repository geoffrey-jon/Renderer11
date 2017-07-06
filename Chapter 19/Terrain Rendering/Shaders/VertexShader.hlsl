//***************************************************************************************
//***************************************************************************************

struct VertexIn
{
	float3 PosL    : POSITION;
	float2 Tex     : TEXCOORD0;
	float2 BoundsY : TEXCOORD1;
};

struct VertexOut
{
    float3 PosW : POSITION;
    float2 Tex : TEXCOORD0;
    float2 BoundsY : TEXCOORD1;
};

Texture2D gHeightMap;

SamplerState samHeightmap
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;

    AddressU = CLAMP;
    AddressV = CLAMP;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Terrain specified directly in world space.
    vout.PosW = vin.PosL;

	// Displace the patch corners to world space.  This is to make 
	// the eye to patch distance calculation more accurate.
    vout.PosW.y = gHeightMap.SampleLevel(samHeightmap, vin.Tex, 0).r;

	// Output vertex attributes to next stage.
    vout.Tex = vin.Tex;
    vout.BoundsY = vin.BoundsY;
	
    return vout;
}
