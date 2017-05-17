/*  ======================
	Summary: Cube Map Demo
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

struct ConstBufferPerObjectDebug
{
	DirectX::XMMATRIX world;
};

struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranpose;
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX texTransform;
	DirectX::XMMATRIX worldViewProjTex;
	Material material;
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
	UINT bUseAO;
	DirectX::XMINT2 pad;
};

struct ConstBufferBlurParams
{
	float texelWidth;
	float texelHeight;
	DirectX::XMFLOAT2 pad;
};

struct ConstBufferPerObjectNormalDepth
{
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldInvTranposeView;
	DirectX::XMMATRIX worldViewProj;
};

struct ConstBufferPerFrameSSAO
{
	DirectX::XMMATRIX viewTex;
	DirectX::XMFLOAT4 frustumFarCorners[4];
	DirectX::XMFLOAT4 offsets[14];
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
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreateVertexShaderNormalDepth(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateVertexShaderSSAO(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawObject(GObject* object);
	void DrawObject(GObject* object, DirectX::XMMATRIX& transform);
	void DrawShadow(GObject* object, DirectX::XMMATRIX& transform);
	void Draw(GObject* object, DirectX::XMMATRIX& world, bool bShadow);

	void RenderScene();

	void RenderNormalDepthMap();
	void RenderSSAOMap();
	void BlurSSAOMap(int numBlurs);
	void DrawSSAOMap();

	void SetupNormalDepth();
	void SetupSSAO();
	void BuildFrustumCorners();
	void BuildOffsetVectors();
	void BuildFullScreenQuad();
	void BuildRandomVectorTexture();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;

	ID3D11Buffer* mConstBufferPerObjectND;
	ID3D11Buffer* mConstBufferPerFrameSSAO;
	ID3D11Buffer* mConstBufferBlurParams;
	ID3D11Buffer* mConstBufferPerObjectDebug;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;

	D3D11_MAPPED_SUBRESOURCE cbPerObjectNDResource;
	D3D11_MAPPED_SUBRESOURCE cbPerFrameSSAOResource;
	D3D11_MAPPED_SUBRESOURCE cbBlurParamsResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectDebugResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;
	
	ConstBufferPerObjectNormalDepth* cbPerObjectND;
	ConstBufferPerFrameSSAO* cbPerFrameSSAO;
	ConstBufferBlurParams* cbBlurParams;
	ConstBufferPerObjectDebug* cbPerObjectDebug;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;

	ID3D11VertexShader* mNormalDepthVS;
	ID3D11PixelShader* mNormalDepthPS;

	ID3D11VertexShader* mSsaoVS;
	ID3D11PixelShader* mSsaoPS;

	ID3D11VertexShader* mBlurVS;
	ID3D11PixelShader* mBlurPS;

	ID3D11VertexShader* mDebugTextureVS;
	ID3D11PixelShader* mDebugTexturePS;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mVertexLayoutNormalDepth;
	ID3D11InputLayout* mVertexLayoutSSAO;

	// Objects
	GObject* mSkullObject;
	GPlaneXZ* mFloorObject;
	GCube* mBoxObject;
	GSphere* mSphereObjects[10];
	GCylinder* mColumnObjects[10];
	GSky* mSkyObject;

	// Lights
	DirectionalLight mDirLights[3];

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// SSAO
	ID3D11RenderTargetView* mNormalDepthRTV;
	ID3D11ShaderResourceView* mNormalDepthSRV;

	ID3D11RenderTargetView* mSsaoRTV0;
	ID3D11RenderTargetView* mSsaoRTV1;

	ID3D11ShaderResourceView* mSsaoSRV0;
	ID3D11ShaderResourceView* mSsaoSRV1;

	DirectX::XMFLOAT4 mFrustumFarCorners[4];
	DirectX::XMFLOAT4 mOffsets[14];

	ID3D11Buffer* mScreenQuadVB;
	ID3D11Buffer* mScreenQuadIB;

	ID3D11ShaderResourceView* mRandomVectorSRV;

	bool mAOSetting;
};

#endif // MYAPP_H