/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GWave.h"

GWave::GWave() : GObject()
{ 
	mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

	mVertexCount = mWaves.VertexCount();
	mIndexCount = mWaves.TriangleCount() * 3;

	mVertices.resize(mVertexCount);
	mIndices.resize(mIndexCount);
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			mIndices[k] = i*n + j;
			mIndices[k + 1] = i*n + j + 1;
			mIndices[k + 2] = (i + 1)*n + j;

			mIndices[k + 3] = (i + 1)*n + j;
			mIndices[k + 4] = i*n + j + 1;
			mIndices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}
}

GWave::~GWave()
{
}

void GWave::Update(float currentTime, float dt, void* data)
{
	static float t_base = 0.0f;
	if ((currentTime - t_base) >= 0.1f)
	{
		t_base += 0.1f;

		DWORD i = 5 + rand() % (mWaves.RowCount() - 10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount() - 10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		mWaves.Disturb(i, j, r);
	}
	mWaves.Update(dt);

	Vertex* v = reinterpret_cast<Vertex*>(data);
	for (UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos = mWaves[i];
		v[i].Normal = mWaves.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].Tex.x = 0.5f + mWaves[i].x / mWaves.Width();
		v[i].Tex.y = 0.5f - mWaves[i].z / mWaves.Depth();
	}

	// Tile water texture.
	DirectX::XMMATRIX wavesScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	mWaterTexOffset.y += 0.05f*dt;
	mWaterTexOffset.x += 0.1f*dt;
	DirectX::XMMATRIX wavesOffset = DirectX::XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&mTexTransform, wavesScale*wavesOffset);
}
