/*  ======================
	======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "RenderStates.h"
#include "GFirstPersonCamera.h"
#include "GObject.h"
#include "GCube.h"
#include "GSphere.h"
#include "GCylinder.h"
#include "GPlaneXZ.h"
#include "GSky.h"
#include "DirectXCollision.h"

struct ConstBufferPerObject
{
	DirectX::XMMATRIX viewProj;
	DirectX::XMMATRIX texTransform;
	Material material;
};

struct ConstBufferWVP
{
	DirectX::XMMATRIX worldViewProj;
};

struct ConstBufferPerFrame
{
	DirectionalLight dirLight0;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
	DirectX::XMFLOAT3 eyePosW;
	float pad;
};

struct ConstBufferPSParams
{
	UINT bUseTexure;
	UINT bAlphaClip;
	UINT bFogEnabled;
	UINT bReflection;
	UINT bUseNormal;
	DirectX::XMINT3 pad;
};

class MyApp : public D3DApp
{
public:
	MyApp(HINSTANCE Instance);
	~MyApp();

	bool Init() override;
	void OnResize() override;

	void UpdateScene(float dt) override;
	void DrawScene() override;

	void OnMouseDown(WPARAM btnState, int x, int y);
	void OnMouseUp(WPARAM btnState, int x, int y);
	void OnMouseMove(WPARAM btnState, int x, int y);
	void OnKeyDown(WPARAM key, LPARAM info);

private:
	void CreateGeometryBuffers(GObject* obj, bool dynamic = false);
	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);

	void CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateSkyVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawObject(GObject* object);
	void DrawObject(GObject* object, DirectX::XMMATRIX& transform);
	void DrawShadow(GObject* object, DirectX::XMMATRIX& transform);
	void Draw(GObject* object, DirectX::XMMATRIX& world, bool bShadow);

	void BuildInstancedBuffer();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;
	ID3D11Buffer* mConstBufferWVP;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;
	D3D11_MAPPED_SUBRESOURCE cbWVPResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;
	ConstBufferWVP* cbWVP;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mSkyVertexLayout;

	// Objects
	GObject* mSkullObject;
	GSky* mSkyObject;

	// Lights
	DirectionalLight mDirLights[3];

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// Instancing
	std::vector<InstancedData> mInstancedData;
	ID3D11Buffer* mInstancedBuffer;
	UINT mVisibleObjectCount;

	// Frustum CUlling
	DirectX::BoundingFrustum mCameraFrustum;
	bool bFrustumCulling;
};

#endif // MYAPP_H