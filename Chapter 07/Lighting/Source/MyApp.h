
/*  =======================
Summary: DX11 Chapter 4
=======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"
#include "Vertex.h"
#include "GObject.h"

struct ConstBufferPerObject
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranpose;
	DirectX::XMMATRIX worldViewProj;
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
	void BuildShapeGeometryBuffers();
	void BuildGeometryBuffers(GObject* obj);

	void BuildVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void BuildPixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);

	void BuildRasterizerState();
	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

private:
	// Data Buffers
	ID3D11Buffer* mShapesVertexBuffer;
	ID3D11Buffer* mShapesIndexBuffer;

	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;

	// Shaders
	ID3D11InputLayout* mInputLayout;

	ID3D11VertexShader* mVertexShader;
	ID3D11PixelShader* mPixelShader;

	DirectionalLight mDirLights[3];
	Material mGridMat;
	Material mBoxMat;

	// State Objects
	ID3D11RasterizerState* mWireframeRS;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;

	UINT mBoxVertexOffset;
	UINT mGridVertexOffset;

	UINT mBoxIndexOffset;
	UINT mGridIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;

	UINT mLightCount;

	DirectX::XMFLOAT3 mEyePosW;

	// Camera
	float mTheta;
	float mPhi;
	float mRadius;

	// User Input
	POINT mLastMousePos;

	// Objects
	GObject* mSkullObject;
};

#endif // MYAPP_H