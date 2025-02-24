/*  ======================
	Summary: Blending Demo
	======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "GObject.h"
#include "GCube.h"
#include "GHill.h"
#include "GWave.h"
#include "GFirstPersonCamera.h"
#include "Waves.h"

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
	DirectionalLight dirLight0;
	DirectionalLight dirLight1;
	DirectionalLight dirLight2;
	DirectX::XMFLOAT3 eyePosW;
	float fogStart;
	float fogRange;
	DirectX::XMFLOAT3 pad;
	DirectX::XMFLOAT4 fogColor;
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
	void CreateRasterizerState();
	void CreateSamplerState();
	void CreateBlendState();

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

	// State Objects
	ID3D11RasterizerState* mWireframeRS;
	ID3D11RasterizerState* mNoCullRS;
	ID3D11SamplerState* mSamplerState;
	ID3D11BlendState* mBlendState;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11InputLayout* mVertexLayout;

	// Lights
	DirectionalLight mDirLights[3];
	UINT mLightCount;

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// Objects
	GCube* mBoxObject;
	GHill* mHillObject;
	GWave* mWaveObject;
};

#endif // MYAPP_H