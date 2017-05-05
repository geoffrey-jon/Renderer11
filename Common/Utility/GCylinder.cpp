/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GCylinder.h"

GCylinder::GCylinder() : GObject()
{ 
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cylinder;
	geoGen.CreateCylinder(0.5f, 0.5f, 3.0f, 15, 15, cylinder);

	mVertexCount = cylinder.Vertices.size();
	mIndexCount = cylinder.Indices.size();

	mVertices.resize(mVertexCount);
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		mVertices[i].Pos = cylinder.Vertices[i].Position;
		mVertices[i].Normal = cylinder.Vertices[i].Normal;
		mVertices[i].Tex = cylinder.Vertices[i].TexC;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), cylinder.Indices.begin(), cylinder.Indices.end());
}

GCylinder::~GCylinder()
{
}
