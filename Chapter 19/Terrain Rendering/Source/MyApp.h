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
#include "GSky.h"
	
struct ConstBufferPerObject
{
	DirectX::XMMATRIX viewProj;
	Material material;
};
	
struct ConstBufferPerFrame
{
	DirectionalLight lights[3];
	DirectX::XMFLOAT3 eyePosW;
	float minDist;
	float maxDist;
	float minTess;
	float maxTess;
	float texelCellSpaceU;
	float texelCellSpaceV;
	float worldCellSpace;
	DirectX::XMFLOAT2 pad;
};

struct ConstBufferWVP
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
	void CreateGeometryBuffers(GObject* obj, bool dynamic = false);
		
	void CreateConstantBuffer(ID3D11Buffer** buffer, UINT size);

	void CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateSkyVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void CreateHullShader(ID3D11HullShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreateDomainShader(ID3D11DomainShader** shader, LPCWSTR filename, LPCSTR entryPoint);
	void CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint);

	void LoadTextureToSRV(ID3D11ShaderResourceView** srv, LPCWSTR filename);

	void InitUserInput();
	void PositionObjects();
	void SetupStaticLights();

	void DrawScene(const GFirstPersonCamera& camera, bool drawSkull);

	void LoadHeightmap(LPCWSTR filename);
	void BuildHeightmapSRV();
	void BuildLayerMapSRV();
	void BuildTerrainBuffers();

private:
	// Constant Buffers
	ID3D11Buffer* mConstBufferPerFrame;
	ID3D11Buffer* mConstBufferPerObject;
	ID3D11Buffer* mConstBufferWVP;

	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	D3D11_MAPPED_SUBRESOURCE cbWVPResource;

	ConstBufferPerFrame* cbPerFrame;
	ConstBufferPerObject* cbPerObject;
	ConstBufferWVP* cbWVP;

	// Shaders
	ID3D11VertexShader* mVertexShader;
	ID3D11HullShader* mHullShader;
	ID3D11DomainShader* mDomainShader;
	ID3D11PixelShader* mPixelShader;

	ID3D11VertexShader* mSkyVertexShader;
	ID3D11PixelShader* mSkyPixelShader;

	// Vertex Layout
	ID3D11InputLayout* mVertexLayout;
	ID3D11InputLayout* mSkyVertexLayout;

	// Objects
	GSky* mSkyObject;

	// Lights
	DirectionalLight mDirLights[3];

	// Camera
	GFirstPersonCamera mCamera;

	// User Input
	POINT mLastMousePos;

	// Terrain Rendering
	ID3D11Buffer* mTerrainVB;
	ID3D11Buffer* mTerrainIB;

	UINT mNumCellsWide;
	UINT mNumCellsDeep;
	FLOAT mCellWidth;
	FLOAT mCellDepth;

	UINT mNumPatchVertRows;
	UINT mNumPatchVertCols;

	UINT mNumPatches;

	UINT mTerrainWidth;
	UINT mTerrainDepth;

	Material mTerrainMaterial;

	std::vector<float> mHeightmap;
	ID3D11ShaderResourceView* mHeightMapSRV;

	ID3D11ShaderResourceView* mLayerMapSRV;
	ID3D11ShaderResourceView* mBlendMapSRV;

	ID3D11Texture2D* mTexArray;
};

#endif // MYAPP_H