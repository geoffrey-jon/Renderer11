//***************************************************************************************
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gViewToTexSpaceTransform;
	float4 gFrustumFarCorners[4];
	float4 gOffsets[14];
};

struct VertexOut
{
	float4 PosH       : SV_POSITION;
	float3 ToFarPlane : TEXCOORD0;
	float2 Tex        : TEXCOORD1;
};

Texture2D gNormalDepthMap;
Texture2D gRandomVectorMap;

SamplerState samNormalDepth
{
	Filter = MIN_MAG_LINEAR_MIP_POINT;

	// Set a very far depth value if sampling outside of the NormalDepth map
	// so we do not get false occlusions.
	AddressU = BORDER;
	AddressV = BORDER;
	BorderColor = float4(0.0f, 0.0f, 0.0f, 1e5f);
};

SamplerState samRandomVec
{
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = WRAP;
    AddressV = WRAP;
};

float PS(VertexOut pin) : SV_Target
{
	// Extract normal and depth info for p from the normal/depth map
	float4 normalDepth = gNormalDepthMap.Sample(samNormalDepth, pin.Tex);
	float3 pn = normalDepth.xyz;
	float  pz = normalDepth.w;

	// Reconstruct 3D view space position for p
	float3 p = (pz / pin.ToFarPlane.z) * pin.ToFarPlane;

	// TODO: Sample from randomVecMap
    float3 randVec = gRandomVectorMap.SampleLevel(samRandomVec, 4.0f * pin.Tex, 0.0f).rgb;
    randVec = (2.0f * randVec) - 1.0f;

	float occlusionSum = 0.0f;

	for (int i = 0; i < 14; ++i)
	{
		float3 offset = gOffsets[i].xyz;

        // TODO: reflect offset across random vector
        offset = reflect(offset, randVec);

		// TODO: flip the sign of any offset vector that is behind the plane defined by (p,n)
        float flip = sign(dot(offset, pn));

        float OcclusionRadius = 0.5f;

		// Offset p to get a potential occluder point
        float3 q = p + offset * OcclusionRadius * flip;

		// Generate projective texture coords for q in order to find it in the depth map
		float4 projQ = mul(float4(q, 1.0f), gViewToTexSpaceTransform);

		// Perspective divide
		projQ /= projQ.w;

		// Sample the normal depth map to get the depth of the nearest visible pixel along the ray from the eye to q
		float rz = gNormalDepthMap.Sample(samNormalDepth, projQ.xy).w;

		// Reconstruct 3D view space position for r
		float3 r = (rz / q.z) * q;

		float occlusion = 0.0f;

		float distZ = p.z - r.z;
		if (distZ > 0.05f)
		{
			float dp = max(dot(pn, normalize(r - p)), 0.0f);
			float fadeLength = 2.0f - 0.2f;
			occlusion = saturate((2.0f - distZ) / fadeLength);
			occlusion = occlusion * dp;
		}
		occlusionSum += occlusion;
	}
	
	occlusionSum /= 14;
	float access = 1.0f - occlusionSum;

	return saturate(pow(access, 4.0f));
}