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
	DirectX::XMINT3 pad;
};

struct ConstBufferPerFrameParticle
{
	DirectX::XMFLOAT4 eyePosW;
	DirectX::XMFLOAT4 emitPosW;
	DirectX::XMFLOAT4 emitDirW;

	float timeStep;
	float gameTime;
	DirectX::XMFLOAT2 pad2;

	DirectX::XMMATRIX viewProj;
};

struct Particle
{
	DirectX::XMFLOAT3 InitialPos;
	DirectX::XMFLOAT3 InitialVel;
	DirectX::XMFLOAT2 Size;
	float Age;
	UINT Type;
};

#define PT_EMITTER 0
#define PT_FLARE 1

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
	void CreateVertexShaderParticle(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreateGeometryShader(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateGeometryShaderStreamOut(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawObject(GObject* object);
	void DrawObject(GObject* object, DirectX::XMMATRIX& transform);
	void DrawShadow(GObject* object, DirectX::XMMATRIX& transform);
	void Draw(GObject* object, DirectX::XMMATRIX& world, bool bShadow);

	void RenderScene();
	void RenderParticleSystem();

	void CreateRandomSRV();
	void BuildParticleVB();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;

	ID3D11VertexShader* mParticleStreamOutVS;
	ID3D11GeometryShader* mParticleStreamOutGS;

	ID3D11VertexShader* mParticleDrawVS;
	ID3D11GeometryShader* mParticleDrawGS;
	ID3D11PixelShader* mParticleDrawPS;


	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mVertexLayoutParticle;

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

	// Fire Particle System
	ID3D11Buffer* mInitVB;
	ID3D11Buffer* mDrawVB;
	ID3D11Buffer* mStreamOutVB;

	ID3D11ShaderResourceView* mTexArraySRV;
	ID3D11ShaderResourceView* mRandomSRV;

	UINT mMaxParticles;
	bool mFirstRun;

	float mGameTime;
	float mTimeStep;
	float mAge;

	DirectX::XMFLOAT3 mEmitPosW;
	DirectX::XMFLOAT3 mEmitDirW;

	ID3D11Buffer* mConstBufferPerFrameParticle;
	D3D11_MAPPED_SUBRESOURCE cbPerFrameParticleResource;
	ConstBufferPerFrameParticle* cbPerFrameParticle;
};

#endif // MYAPP_H