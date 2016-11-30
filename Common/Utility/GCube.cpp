/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GCube.h"

GCube::GCube() : GObject()
{ 
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData cube;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, cube);

	mVertexCount = cube.Vertices.size();
	mIndexCount = cube.Indices.size();

	mVertices.resize(mVertexCount);
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		mVertices[i].Pos = cube.Vertices[i].Position;
		mVertices[i].Normal = cube.Vertices[i].Normal;
		mVertices[i].Tex = cube.Vertices[i].TexC;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), cube.Indices.begin(), cube.Indices.end());
}

GCube::~GCube()
{
}
