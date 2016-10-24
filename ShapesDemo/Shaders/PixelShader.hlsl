//***************************************************************************************
// color.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

struct VertexOut
{
	float4 PosH  : SV_POSITION;
    float4 Color : COLOR;
};

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}
