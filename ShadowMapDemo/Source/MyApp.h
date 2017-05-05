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
#include "ShadowMap.h"

struct ConstBufferPerObjectShadow
{
	DirectX::XMMATRIX worldViewProj;
};

struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranpose;
	DirectX::XMMATRIX worldViewProj;
	DirectX::XMMATRIX texTransform;
	DirectX::XMMATRIX shadowTransform;
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

	void CreateVertexShaderShadow(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();
	void BuildShadowTransform();

	void DrawObject(GObject* object);
	void DrawObject(GObject* object, DirectX::XMMATRIX& transform);
	void DrawShadow(GObject* object, DirectX::XMMATRIX& transform);
	void Draw(GObject* object, DirectX::XMMATRIX& world, bool bShadow);

	void RenderScene();
	void RenderShadowMap();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferPSParams;
	ID3D11Buffer* mConstBufferPerObjectShadow;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbPSParamsResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectShadowResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferPSParams* cbPSParams;
	ConstBufferPerObjectShadow* cbPerObjectShadow;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;

	ID3D11VertexShader* mShadowVertexShader;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mVertexLayoutShadow;

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
	bool bPicked;
	bool mNormalSetting;

	// Shadow Map
	ShadowMap* mShadowMap;
	float mLightRotationAngle;
	DirectX::XMFLOAT3 mOriginalLightDir[3];

	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;
};

#endif // MYAPP_H