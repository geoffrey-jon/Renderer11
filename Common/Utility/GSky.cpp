/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GSky.h"

GSky::GSky(float skySphereRadius) : GObject()
{ 
	GeometryGenerator::MeshData sphere;
	GeometryGenerator geoGen;
	geoGen.CreateSphere(skySphereRadius, 30, 30, sphere);

	mVertexCount = sphere.Vertices.size();
	mIndexCount = sphere.Indices.size();

	mVertices.resize(mVertexCount);

	for (size_t i = 0; i < mVertexCount; ++i)
	{
		mVertices[i].Pos = sphere.Vertices[i].Position;
	}

	mIndices.resize(mIndexCount);
	mIndices.insert(mIndices.begin(), sphere.Indices.begin(), sphere.Indices.end());
}

void GSky::SetEyePos(float x, float y, float z)
{
	DirectX::XMStoreFloat4x4(&mWorldTransform, DirectX::XMMatrixTranslation(x, y, z));
}

// TODO: OVERRIDE TRANSFORMATION FUNCTIONS TO HAVE NO MEANING FOR GSKY

GSky::~GSky()
{
}
