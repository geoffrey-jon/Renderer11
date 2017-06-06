//***************************************************************************************
//***************************************************************************************

static const float gWeights[11] = { 0.05f, 0.05f, 0.1f, 0.1f, 0.1f, 0.2f, 0.1f, 0.1f, 0.1f, 0.05f, 0.05f };
static const int gBlurRadius = 5;

#define N 256
#define CacheSize (N + 2*gBlurRadius)
groupshared float4 gCache[CacheSize];

Texture2D gInput;
RWTexture2D<float4> gOutput;

[numthreads(N, 1, 1)]
void CS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    float index = groupThreadID.x;
    float2 coord = dispatchThreadID.xy;

    uint imageWidth;
    uint imageHeight;
    gInput.GetDimensions(imageWidth, imageHeight);

    // Leftmost R pixels
    if (index < gBlurRadius)
    {
        int x = max(coord.x - gBlurRadius, 0);
        gCache[index] = gInput[int2(x, coord.y)];
    }

    // Rightmost R pixels
    if (index >= N - gBlurRadius)
    {
        int x = min(coord.x + gBlurRadius, imageWidth - 1);
        gCache[index + 2*gBlurRadius] = gInput[int2(x, coord.y)];
    }

    int x = min(coord.x, imageWidth - 1);
    gCache[index + gBlurRadius] = gInput[float2(x, coord.y)];

    GroupMemoryBarrierWithGroupSync();

    // Blur each pixel
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);

    [unroll]
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = index + gBlurRadius + i;
        blurColor += gCache[k] * gWeights[i + gBlurRadius];
    }

    gOutput[coord] = blurColor;
}
