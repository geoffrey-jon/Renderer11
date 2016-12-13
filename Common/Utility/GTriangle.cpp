/*  =======================
	Summary: 
	=======================  */

#include "GTriangle.h"

GTriangle::GTriangle() : GObject()
{
	mIndexCount = 0;
	mIndices.resize(0);

	Vertex v0, v1, v2;
	v0.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	v0.Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	v1.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	v1.Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	v2.Pos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	v2.Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);

	mVertexCount = 3;
	mVertices.resize(3);

	mVertices[0] = v0;
	mVertices[1] = v1;
	mVertices[2] = v2;
}

GTriangle::GTriangle(Vertex v0, Vertex v1, Vertex v2) : GObject()
{
	mIndexCount = 0;
	mVertexCount = 3;

	mIndices.resize(0);
	mVertices.resize(3);

	mVertices[0] = v0;
	mVertices[1] = v1;
	mVertices[2] = v2;
}

GTriangle::~GTriangle()
{
}

void GTriangle::SetVertices(Vertex v0, Vertex v1, Vertex v2)
{
	mVertices.resize(3);
	mVertices[0] = v0;
	mVertices[1] = v1;
	mVertices[2] = v2;
}

void GTriangle::Update(void* data)
{
	Vertex* v = reinterpret_cast<Vertex*>(data);
	for (UINT i = 0; i < 3; ++i)
	{
		v[i] = mVertices[i];
	}
}