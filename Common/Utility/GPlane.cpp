/*  =======================
Summary: DX11 Chapter 4
=======================  */

#include "GPlane.h"

GPlane::GPlane() : GObject()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData plane;
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, plane);

	mVertexCount = plane.Vertices.size();
	mIndexCount = plane.Indices.size();

	mVertices.resize(mVertexCount);
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		mVertices[i].Pos = plane.Vertices[i].Position;
		mVertices[i].Normal = plane.Vertices[i].Normal;
		mVertices[i].Tex = plane.Vertices[i].TexC;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), plane.Indices.begin(), plane.Indices.end());
}

GPlane::~GPlane()
{
}
