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
	
struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;
};
	
struct ConstBufferPerFrame
{
	DirectX::XMFLOAT3 eyePosW;
	float pad;
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
	void CreateGeometryBuffers(GObject* obj, bool dynamic = false);
		
	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);

	void CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateHullShader(ID3D11HullShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateDomainShader(ID3D11DomainShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawObject(GObject* object, const GFirstPersonCamera* camera);
	void DrawObject(GObject* object, const GFirstPersonCamera* camera, DirectX::XMMATRIX& transform);
	void DrawShadow(GObject* object, const GFirstPersonCamera* camera, DirectX::XMMATRIX& transform);
	void Draw(GObject* object, const GFirstPersonCamera* camera, DirectX::XMMATRIX& world, bool bShadow);

	void DrawScene(const GFirstPersonCamera& camera, bool drawSkull);

	void BuildTrianglePatchBuffer();

private:
	ID3D11Buffer* mTriPatchVB;

	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11HullShader* mHullShader;
	ID3D11DomainShader* mDomainShader;
	ID3D11PixelShader* mPixelShader;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;
};

#endif // MYAPP_H