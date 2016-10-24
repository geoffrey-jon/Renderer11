/*  =======================
	Summary: DX11 Chapter 4 
	=======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT4 Color;
};

struct InstancedVertex
{
	DirectX::XMFLOAT4 TexCoord0;
	DirectX::XMFLOAT4 TexCoord1;
	DirectX::XMFLOAT4 TexCoord2;
	DirectX::XMFLOAT4 TexCoord3;
};

struct ConstBuffer
{
	DirectX::XMMATRIX worldViewProj;
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
	void BuildGeometryBuffers();
	void BuildShaders();
	void BuildRasterizerState();
	void InitUserInput();
	void PositionObjects();

private:
	// Data Buffers
	ID3D11Buffer* mVertexBuffer;
	ID3D11Buffer* mInstanceBuffer;
	ID3D11Buffer* mIndexBuffer;
	ID3D11Buffer* mConstBuffer;

	// Shaders
	ID3D11InputLayout* mInputLayout;
	ID3D11InputLayout* mInstancedInputLayout;

	ID3D11VertexShader* mVertexShader;
	ID3D11VertexShader* mInstancedVertexShader;

	ID3D11PixelShader* mPixelShader;

	// State Objects
	ID3D11RasterizerState* mWireframeRS;

	DirectX::XMFLOAT4X4 mView;
	DirectX::XMFLOAT4X4 mProj;

	// Define transformations from local spaces to world space.
	DirectX::XMFLOAT4X4 mBoxWorld;
	DirectX::XMFLOAT4X4 mGridWorld;
	DirectX::XMFLOAT4X4 mCenterSphere;

	int mBoxVertexOffset;
	int mGridVertexOffset;
	int mSphereVertexOffset;
	int mCylinderVertexOffset;

	UINT mBoxIndexOffset;
	UINT mGridIndexOffset;
	UINT mSphereIndexOffset;
	UINT mCylinderIndexOffset;

	UINT mBoxIndexCount;
	UINT mGridIndexCount;
	UINT mSphereIndexCount;
	UINT mCylinderIndexCount;

	// Camera
	float mTheta;
	float mPhi;
	float mRadius;

	// User Input
	POINT mLastMousePos;
};

#endif // MYAPP_H