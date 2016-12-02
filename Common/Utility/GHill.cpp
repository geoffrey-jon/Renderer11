/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GHill.h"

GHill::GHill() : GObject()
{ 
	GeometryGenerator::MeshData grid;

	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mVertexCount = grid.Vertices.size();
	mIndexCount = grid.Indices.size();

	mVertices.resize(mVertexCount);
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		DirectX::XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);

		mVertices[i].Pos = p;
		mVertices[i].Normal = GetHillNormal(p.x, p.z);
		mVertices[i].Tex = grid.Vertices[i].TexC;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), grid.Indices.begin(), grid.Indices.end());
}

GHill::~GHill()
{
}

float GHill::GetHillHeight(float x, float z)const
{
	return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

DirectX::XMFLOAT3 GHill::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	DirectX::XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}
