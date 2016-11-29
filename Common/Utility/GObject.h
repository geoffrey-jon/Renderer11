/*  =======================
	Summary: DX11 Chapter 4 
	=======================  */

#ifndef GOBJECT_H
#define GOBJECT_H

#include "D3D11.h"
#include "LightHelper.h"
#include "Vertex.h"
#include <string>
#include <vector>

__declspec(align(16))
class GObject
{
public:
	GObject(std::string filename);
	~GObject();


	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

	bool Init();

	inline Material GetMaterial() { return mMaterial; }
	void SetMaterial(Material mat);
	void SetAmbient(DirectX::XMFLOAT4 ambient);
	void SetDiffuse(DirectX::XMFLOAT4 diffuse);
	void SetSpecular(DirectX::XMFLOAT4 specular);
	void SetReflect(DirectX::XMFLOAT4 reflect);

	void Translate(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);

	inline UINT GetIndexCount() { return mIndexCount; }
	inline UINT GetVertexCount() { return mVertexCount; }

	inline void* GetIndices() { return &mIndices[0]; }
	inline void* GetVertices() { return &mVertices[0]; }

	inline ID3D11Buffer** GetIndexBuffer() { return &mIndexBuffer; }
	inline ID3D11Buffer** GetVertexBuffer() { return &mVertexBuffer; }

	void SetWorldTransform(DirectX::XMFLOAT4X4 transform);
	DirectX::XMFLOAT4X4 GetWorldTransform();

private:
	bool ReadObjFile();

private:
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;

	std::vector<Vertex> mVertices;
	std::vector<UINT> mIndices;

	std::string mFilename;

	Material mMaterial;

	DirectX::XMFLOAT4X4 mWorldTransform;

	DirectX::XMMATRIX mTranslation;
	DirectX::XMMATRIX mRotation;
	DirectX::XMMATRIX mScale;

	UINT mIndexCount;
	UINT mVertexCount;
};

#endif // GOBJECT_H