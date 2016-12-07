/*  ======================
	Summary: Stencil Demo
	======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "GObject.h"
#include "GPlaneXY.h"
#include "GPlaneXZ.h"
#include "GFirstPersonCamera.h"

struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranpose;
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX texTransform;
	Material material;
};

struct ConstBufferPerFrame
{
	DirectX::XMFLOAT3 eyePosW;
	float fogStart;
	float fogRange;
	DirectX::XMFLOAT3 pad;
	DirectX::XMFLOAT4 fogColor;
};

struct ConstBufferLights
{
	DirectionalLight dirLight0;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
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

private:
	void InitRasterizerStates();
	void InitSamplerStates();
	void InitBlendStates();
	void InitDepthStencilStates();

	void CreateGeometryBuffers(GObject* obj, bool dynamic = false);

	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);
	void CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferLights;

	// State Objects
	ID3D11RasterizerState* mCullClockwiseRS;

	ID3D11BlendState*      mTransparentBS;
	ID3D11BlendState*      mNoRenderTargetWritesBS;

	ID3D11DepthStencilState* mMarkMirrorDSS;
	ID3D11DepthStencilState* mDrawReflectionDSS;
	ID3D11DepthStencilState* mNoDoubleBlendDSS;

	ID3D11SamplerState* mSamplerState;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;
	ID3D11PixelShader* mPixelShaderNoTexture;

	ID3D11InputLayout* mVertexLayout;

	// Lights
	DirectionalLight mDirLights[3];

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// Objects
	GObject* mSkullObject;
	GPlaneXZ* mFloorObject;
	GPlaneXY* mWallPiece1;
	GPlaneXY* mWallPiece2;
	GPlaneXY* mWallPiece3;
	GPlaneXY* mMirrorObject;

	Material mShadowMat;
};

#endif // MYAPP_H