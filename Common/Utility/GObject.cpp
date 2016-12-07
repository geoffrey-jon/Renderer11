/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "GObject.h"
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
	}

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

