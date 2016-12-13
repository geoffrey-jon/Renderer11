/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GObject.h"
#include "GTriangle.h"
#include "D3DUtil.h"

GObject::GObject()
{
	Init();
}

GObject::GObject(std::string filename)
{ 
	mFilename = filename;
	ReadObjFile();
	Init();
}

GObject::~GObject()
{
	ReleaseCOM(mVertexBuffer);
	ReleaseCOM(mIndexBuffer);
	ReleaseCOM(mDiffuseMapSRV);
}

bool GObject::Init()
{
	mVertexBuffer = nullptr; 
	mIndexBuffer = nullptr;
	mDiffuseMapSRV = nullptr;
	mTranslation = DirectX::XMMatrixIdentity();
	mRotation = DirectX::XMMatrixIdentity();
	mScale = DirectX::XMMatrixIdentity();
	DirectX::XMStoreFloat4x4(&mTexTransform, DirectX::XMMatrixIdentity());
	return true;
}

bool GObject::ReadObjFile()
{
	DirectX::XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	DirectX::XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

	DirectX::XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	DirectX::XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

	std::ifstream fin(mFilename);

	if (!fin) { return false; }

	std::string ignore;

	fin >> ignore >> mVertexCount;
	fin >> ignore >> mIndexCount;
	fin >> ignore >> ignore >> ignore >> ignore;

	mVertices.resize(mVertexCount);
	for (UINT i = 0; i < mVertexCount; ++i)
	{
		fin >> mVertices[i].Pos.x >> mVertices[i].Pos.y >> mVertices[i].Pos.z;
		fin >> mVertices[i].Normal.x >> mVertices[i].Normal.y >> mVertices[i].Normal.z;

		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&mVertices[i].Pos);

		vMin = DirectX::XMVectorMin(vMin, P);
		vMax = DirectX::XMVectorMax(vMax, P);
	}
	
	DirectX::XMStoreFloat3(&mAABB.Center, DirectX::XMVectorScale((DirectX::XMVectorAdd(vMin, vMax)), 0.5f));
	DirectX::XMStoreFloat3(&mAABB.Extents, DirectX::XMVectorScale((DirectX::XMVectorAdd(DirectX::XMVectorNegate(vMin), vMax)), 0.5f));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	mIndices.resize(mIndexCount * 3);
	for (UINT i = 0; i < mIndexCount; ++i)
	{
		fin >> mIndices[i * 3 + 0] >> mIndices[i * 3 + 1] >> mIndices[i * 3 + 2];
	}
	mIndexCount = mIndexCount * 3;

	fin.close();
	return true;
}

void GObject::SetMaterial(Material mat)
{
	mMaterial.Ambient = mat.Ambient;
	mMaterial.Diffuse = mat.Diffuse;
	mMaterial.Specular = mat.Specular;
	mMaterial.Reflect = mat.Reflect;
}

void GObject::SetAmbient(DirectX::XMFLOAT4 ambient)
{
	mMaterial.Ambient = ambient;
}

void GObject::SetDiffuse(DirectX::XMFLOAT4 diffuse)
{
	mMaterial.Diffuse = diffuse;
}

void GObject::SetSpecular(DirectX::XMFLOAT4 specular)
{
	mMaterial.Specular = specular;
}

void GObject::SetReflect(DirectX::XMFLOAT4 reflect)
{
	mMaterial.Reflect = reflect;
}

void GObject::Translate(float x, float y, float z)
{
	mTranslation = DirectX::XMMatrixTranslation(x, y, z);
}

void GObject::Rotate(float x, float y, float z)
{
	mRotation = DirectX::XMMatrixRotationRollPitchYaw(
		DirectX::XMConvertToRadians(x),
		DirectX::XMConvertToRadians(y), 
		DirectX::XMConvertToRadians(z));
}

void GObject::Scale(float x, float y, float z)
{
	mScale = DirectX::XMMatrixScaling(x, y, z);
}

DirectX::XMFLOAT4X4 GObject::GetWorldTransform() 
{
	DirectX::XMMATRIX SR = XMMatrixMultiply(mScale, mRotation);
	XMStoreFloat4x4(&mWorldTransform, XMMatrixMultiply(SR, mTranslation));
	return mWorldTransform;
}

DirectX::XMFLOAT4X4 GObject::GetTexTransform()
{
	return mTexTransform;
}

void GObject::SetTextureScaling(float x, float y)
{
	DirectX::XMMATRIX grassTexScale = DirectX::XMMatrixScaling(x, y, 0.0f);
	XMStoreFloat4x4(&mTexTransform, grassTexScale);
}

bool GObject::Pick(const DirectX::XMVECTOR& rayOriginV, const DirectX::XMVECTOR& rayDirectionV, const DirectX::XMMATRIX& invView, GTriangle* pickedTri)
{
	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&mWorldTransform);
	DirectX::XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

	DirectX::XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

	DirectX::XMVECTOR rayOriginL = XMVector3TransformCoord(rayOriginV, toLocal);
	DirectX::XMVECTOR rayDirectionL = XMVector3TransformNormal(rayDirectionV, toLocal);

	// Make the ray direction unit length for the intersection tests.
	rayDirectionL = DirectX::XMVector3Normalize(rayDirectionL);

	// If we hit the bounding box of the Mesh, then we might have picked a Mesh triangle,
	// so do the ray/triangle tests.
	//
	// If we did not hit the bounding box, then it is impossible that we hit 
	// the Mesh, so do not waste effort doing ray/triangle tests.

	// Assume we have not picked anything yet, so init to -1.
	int mPickedTriangle = -1;
	float tmin = 0.0f;
	if (mAABB.Intersects(rayOriginL, rayDirectionL, tmin))
	{
		// Find the nearest ray/triangle intersection.
		tmin = MathHelper::Infinity;
		for (UINT i = 0; i < mIndexCount / 3; ++i)
		{
			// Indices for this triangle.
			UINT i0 = mIndices[i * 3 + 0];
			UINT i1 = mIndices[i * 3 + 1];
			UINT i2 = mIndices[i * 3 + 2];

			// Vertices for this triangle.
			DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&mVertices[i0].Pos);
			DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&mVertices[i1].Pos);
			DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&mVertices[i2].Pos);

			// We have to iterate over all the triangles in order to find the nearest intersection.
			float t = 0.0f;
			if (DirectX::TriangleTests::Intersects(rayOriginL, rayDirectionL, v0, v1, v2, t))
			{
				if (t < tmin)
				{
					// This is the new nearest picked triangle.
					tmin = t;
					mPickedTriangle = i;
				}
			}
		}

		if (mPickedTriangle != -1)
		{
			UINT i0 = mIndices[mPickedTriangle * 3 + 0];
			UINT i1 = mIndices[mPickedTriangle * 3 + 1];
			UINT i2 = mIndices[mPickedTriangle * 3 + 2];

			pickedTri->SetVertices(mVertices[i0], mVertices[i1], mVertices[i2]);
			return true;
		}
	}
	return false;
}
