/*  =======================
	Summary: 
	=======================  */

#include "GPlaneYZ.h"

GPlaneYZ::GPlaneYZ() : GObject()
{
	GPlaneYZ(10.0f, 10.0f, 10, 10);
}

GPlaneYZ::GPlaneYZ(float height, float depth, UINT m, UINT n) : GObject()
{
	UINT faceCount = (m - 1) * (n - 1) * 2;
	mIndexCount = faceCount * 3;
	mVertexCount = m * n;

	// Create the vertices.
	float halfHeight = 0.5f * height;
	float halfDepth = 0.5f * depth;

	float dx = height / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	mVertices.resize(mVertexCount);
	for (UINT i = 0; i < m; ++i)
	{
		float z = halfDepth - (i * dz);
		for (UINT j = 0; j < n; ++j)
		{
			float y = (j * dx) - halfHeight;

			mVertices[(i * n) + j].Pos = DirectX::XMFLOAT3(0.0f, y, z);
			mVertices[(i * n) + j].Normal = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
//			mVertices[(i * n) + j].TangentU = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);

			// Stretch texture over grid.
			mVertices[(i * n) + j].Tex.x = j * du;
			mVertices[(i * n) + j].Tex.y = i * dv;
		}
	}

	// Create the indices.
	mIndices.resize(mIndexCount); // 3 indices per face

	UINT k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (UINT j = 0; j < n - 1; ++j)
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

GPlaneYZ::~GPlaneYZ()
{
}
