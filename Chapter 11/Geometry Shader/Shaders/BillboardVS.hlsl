//=============================================================================
//=============================================================================

struct VertexIn
{
    float3 CenterW : POSITION;
    float2 SizeW   : SIZE;
};

struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW   : SIZE;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.CenterW = vin.CenterW;
    vout.SizeW = vin.SizeW;
    	
    return vout;
}
