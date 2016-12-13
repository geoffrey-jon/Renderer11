/*  =======================
	Summary: DX11 Chapter 4 
	=======================  */

#ifndef GOBJECT_H
#define GOBJECT_H

#include "D3D11.h"
#include "LightHelper.h"
#include "Vertex.h"
#include "DirectXCollision.h"
#include <string>
#include <vector>

class GTriangle;

__declspec(align(16))
class GObject
{
public:
	GObject();
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

	void SetTexture(LPCWSTR filename);
	void SetTextureScaling(float x, float y);

	void Translate(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);

	inline UINT GetIndexCount() { return mIndexCount; }
	inline UINT GetVertexCount() { return mVertexCount; }

	inline void* GetIndices() { return &mIndices[0]; }
	inline void* GetVertices() { return &mVertices[0]; }

	inline ID3D11Buffer** GetIndexBuffer() { return &mIndexBuffer; }
	inline ID3D11Buffer** GetVertexBuffer() { return &mVertexBuffer; }
	inline ID3D11ShaderResourceView** GetDiffuseMapSRV() { return &mDiffuseMapSRV; }

	DirectX::XMFLOAT4X4 GetWorldTransform();
	DirectX::XMFLOAT4X4 GetTexTransform();

	bool Pick(const DirectX::XMVECTOR& rayOriginV, 
		      const DirectX::XMVECTOR& rayDirectionV, 
		      const DirectX::XMMATRIX& invView,
		      GTriangle* pickedTri);

private:
	bool ReadObjFile();

protected:
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mIndexBuffer;

	ID3D11ShaderResourceView* mDiffuseMapSRV;

	DirectX::BoundingBox mAABB;

	std::vector<Vertex> mVertices;
	std::vector<UINT> mIndices;

	std::string mFilename;

	Material mMaterial;

	DirectX::XMFLOAT4X4 mWorldTransform;
	DirectX::XMFLOAT4X4 mTexTransform;

	DirectX::XMMATRIX mTranslation;
	DirectX::XMMATRIX mRotation;
	DirectX::XMMATRIX mScale;

	UINT mIndexCount;
	UINT mVertexCount;
};

#endif // GOBJECT_H