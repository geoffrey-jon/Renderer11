/*  =======================
	Summary: 
	=======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "GObject.h"
#include "GCube.h"
#include "GPlane.h"
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

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
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

	void CreateGeometryBuffers(GObject* obj);

	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);
	void CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	float GetHillHeight(float x, float z)const;
	DirectX::XMFLOAT3 GetHillNormal(float x, float z)const;

	void BuildLandGeometryBuffers();
	void BuildWaveGeometryBuffers();
	void BuildCrateGeometryBuffers();

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



	ID3D11Buffer* mLandVB;
	ID3D11Buffer* mLandIB;

	ID3D11Buffer* mWavesVB;
	ID3D11Buffer* mWavesIB;

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	ID3D11ShaderResourceView* mGrassMapSRV;
	ID3D11ShaderResourceView* mWavesMapSRV;
	ID3D11ShaderResourceView* mBoxMapSRV;

	Waves mWaves;

	Material mLandMat;
	Material mWavesMat;
	Material mBoxMat;

	DirectX::XMFLOAT4X4 mGrassTexTransform;
	DirectX::XMFLOAT4X4 mWaterTexTransform;
	DirectX::XMFLOAT4X4 mLandWorld;
	DirectX::XMFLOAT4X4 mWavesWorld;
	DirectX::XMFLOAT4X4 mBoxWorld;

	UINT mLandIndexCount;

	DirectX::XMFLOAT2 mWaterTexOffset;
};

#endif // MYAPP_H