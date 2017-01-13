/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GSphere.h"

GSphere::GSphere() : GObject()
{ 
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData sphere;
	geoGen.CreateSphere(0.5f, 20, 20, sphere);

	mVertexCount = sphere.Vertices.size();
	mIndexCount = sphere.Indices.size();

	mVertices.resize(mVertexCount);
	for (size_t i = 0; i < mVertexCount; ++i)
	{
		mVertices[i].Pos = sphere.Vertices[i].Position;
		mVertices[i].Normal = sphere.Vertices[i].Normal;
		mVertices[i].Tex = sphere.Vertices[i].TexC;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), sphere.Indices.begin(), sphere.Indices.end());
}

GSphere::~GSphere()
{
}
