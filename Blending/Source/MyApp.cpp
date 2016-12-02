/*  =============================================
	Summary: First Person Perspective Camera Demo
	=============================================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance),
	mConstBufferPerFrame(0),
	mConstBufferPerObject(0),
	mVertexShader(0),
	mPixelShader(0),
	mLightCount(1),
	mVertexLayout(0),
	mNoCullRS(0),
	mWireframeRS(0),
	mSamplerState(0),
	mLandVB(0),
	mLandIB(0),
	mWavesVB(0),
	mWavesIB(0),
	mBoxVB(0),
	mBoxIB(0),
	mGrassMapSRV(0), 
	mWavesMapSRV(0), 
	mBoxMapSRV(0),
	mWaterTexOffset(0.0f, 0.0f),
	mLandIndexCount(0)
{
	mWindowTitle = L"Blending Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();
	ReleaseCOM(mLandVB);
	ReleaseCOM(mLandIB);
	ReleaseCOM(mWavesVB);
	ReleaseCOM(mWavesIB);
	ReleaseCOM(mBoxVB);
	ReleaseCOM(mBoxIB);
	ReleaseCOM(mGrassMapSRV);
	ReleaseCOM(mWavesMapSRV);
	ReleaseCOM(mBoxMapSRV);
	ReleaseCOM(mBlendState);

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexLayout);
	ReleaseCOM(mNoCullRS);
	ReleaseCOM(mWireframeRS);
	ReleaseCOM(mSamplerState);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initialize Camera
	mCamera.SetPosition(30.0f, 50.0f, -100.0f);
	mCamera.RotateY(-MathHelper::Pi / 8.0);
	mCamera.Pitch(MathHelper::Pi / 8.0);

	// Initialize User Input
	InitUserInput();

	// Initialize Waves
	mWaves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

	LoadTextureToSRV(&mGrassMapSRV, L"Textures/grass.dds");
	LoadTextureToSRV(&mWavesMapSRV, L"Textures/water2.dds");
	LoadTextureToSRV(&mBoxMapSRV, L"Textures/wirefence.dds");

	BuildLandGeometryBuffers();
	BuildWaveGeometryBuffers();
	BuildCrateGeometryBuffers();

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));

	// Construct and Bind the Rasterizer State
	CreateRasterizerState();
	CreateSamplerState();
	CreateBlendState();

	SetupStaticLights();

	return true;
}

void MyApp::InitUserInput()
{
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;
}

void MyApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mMainWindow);
}

void MyApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void MyApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

		mCamera.Pitch(dy);
		mCamera.RotateY(dx);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MyApp::CreateGeometryBuffers(GObject* obj)
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * obj->GetVertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = obj->GetVertices();
	HR(mDevice->CreateBuffer(&vbd, &vinitData, obj->GetVertexBuffer()));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * obj->GetIndexCount();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = obj->GetIndices();
	HR(mDevice->CreateBuffer(&ibd, &iinitData, obj->GetIndexBuffer()));
}

float MyApp::GetHillHeight(float x, float z)const
{
	return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

DirectX::XMFLOAT3 MyApp::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	DirectX::XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	DirectX::XMVECTOR unitNormal = DirectX::XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void MyApp::BuildLandGeometryBuffers()
{
	GeometryGenerator::MeshData grid;

	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	mLandIndexCount = grid.Indices.size();

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (UINT i = 0; i < grid.Vertices.size(); ++i)
	{
		DirectX::XMFLOAT3 p = grid.Vertices[i].Position;

		p.y = GetHillHeight(p.x, p.z);

		vertices[i].Pos = p;
		vertices[i].Normal = GetHillNormal(p.x, p.z);
		vertices[i].Tex = grid.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * grid.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mLandVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * mLandIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.Indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mLandIB));
}

void MyApp::BuildWaveGeometryBuffers()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DYNAMIC;
	vbd.ByteWidth = sizeof(Vertex) * mWaves.VertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbd.MiscFlags = 0;
	HR(mDevice->CreateBuffer(&vbd, 0, &mWavesVB));


	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	std::vector<UINT> indices(3 * mWaves.TriangleCount()); // 3 indices per face

														   // Iterate over each quad.
	UINT m = mWaves.RowCount();
	UINT n = mWaves.ColumnCount();
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
		{
			indices[k] = i*n + j;
			indices[k + 1] = i*n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i*n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mWavesIB));
}

void MyApp::BuildCrateGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(box.Vertices.size());

	for (UINT i = 0; i < box.Vertices.size(); ++i)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * box.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * box.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &box.Indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
}

void MyApp::PositionObjects()
{
	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mLandWorld, I);
	XMStoreFloat4x4(&mWavesWorld, I);

	DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(15.0f, 15.0f, 15.0f);
	DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(8.0f, 5.0f, -15.0f);
	XMStoreFloat4x4(&mBoxWorld, boxScale*boxOffset);

	DirectX::XMMATRIX grassTexScale = DirectX::XMMatrixScaling(10.0f, 10.0f, 0.0f);
	XMStoreFloat4x4(&mGrassTexTransform, grassTexScale);

	mLandMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mLandMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mLandMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	mWavesMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mWavesMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	mWavesMat.Specular = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	mBoxMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

void MyApp::CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayout));

	VSByteCode->Release();
}

void MyApp::CreatePixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* PSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ps_5_0", D3DCOMPILE_DEBUG, 0, &PSByteCode, 0));

	HR(mDevice->CreatePixelShader(PSByteCode->GetBufferPointer(), PSByteCode->GetBufferSize(), NULL, shader));

	PSByteCode->Release();
}

void MyApp::LoadTextureToSRV(ID3D11ShaderResourceView** SRV, LPCWSTR filename)
{
	ID3D11Resource* texResource = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(mDevice, filename, &texResource, SRV));
	ReleaseCOM(texResource); // view saves reference
}

void MyApp::CreateConstantBuffer(ID3D11Buffer** buffer, UINT size)
{
	D3D11_BUFFER_DESC desc;

	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = size;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	HR(mDevice->CreateBuffer(&desc, NULL, buffer));
}

void MyApp::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_SOLID;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(mDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));

	D3D11_RASTERIZER_DESC noCullDesc;
	ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = false;
	noCullDesc.DepthClipEnable = true;

	HR(mDevice->CreateRasterizerState(&noCullDesc, &mNoCullRS));
}

void MyApp::CreateSamplerState()
{
	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 4;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SamplerDesc.BorderColor[0] = 1.0f;
	SamplerDesc.BorderColor[1] = 1.0f;
	SamplerDesc.BorderColor[2] = 1.0f;
	SamplerDesc.BorderColor[3] = 1.0f;
	SamplerDesc.MinLOD = -FLT_MAX;
	SamplerDesc.MaxLOD = FLT_MAX;

	mDevice->CreateSamplerState(&SamplerDesc, &mSamplerState);
}

void MyApp::CreateBlendState()
{
	D3D11_BLEND_DESC blendDesc = { 0 };
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;

	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(mDevice->CreateBlendState(&blendDesc, &mBlendState));
}

void MyApp::SetupStaticLights()
{
	mDirLights[0].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	mDirLights[1].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[1].Diffuse = DirectX::XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	mDirLights[2].Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Diffuse = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[2].Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mDirLights[2].Direction = DirectX::XMFLOAT3(0.0f, -0.707f, -0.707f);
}

void MyApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void MyApp::UpdateScene(float dt)
{
	//
	// Control the camera.
	//
	if (GetAsyncKeyState('W') & 0x8000)
		mCamera.Walk(10.0f*dt);

	if (GetAsyncKeyState('S') & 0x8000)
		mCamera.Walk(-10.0f*dt);

	if (GetAsyncKeyState('A') & 0x8000)
		mCamera.Strafe(-10.0f*dt);

	if (GetAsyncKeyState('D') & 0x8000)
		mCamera.Strafe(10.0f*dt);

	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if ((mTimer.TotalTime() - t_base) >= 0.1f)
	{
		t_base += 0.1f;

		DWORD i = 5 + rand() % (mWaves.RowCount() - 10);
		DWORD j = 5 + rand() % (mWaves.ColumnCount() - 10);

		float r = MathHelper::RandF(0.5f, 1.0f);

		mWaves.Disturb(i, j, r);
	}

	mWaves.Update(dt);

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(mImmediateContext->Map(mWavesVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex* v = reinterpret_cast<Vertex*>(mappedData.pData);
	for (UINT i = 0; i < mWaves.VertexCount(); ++i)
	{
		v[i].Pos = mWaves[i];
		v[i].Normal = mWaves.Normal(i);

		// Derive tex-coords in [0,1] from position.
		v[i].Tex.x = 0.5f + mWaves[i].x / mWaves.Width();
		v[i].Tex.y = 0.5f - mWaves[i].z / mWaves.Depth();
	}

	mImmediateContext->Unmap(mWavesVB, 0);

	//
	// Animate water texture coordinates.
	//

	// Tile water texture.
	DirectX::XMMATRIX wavesScale = DirectX::XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time.
	mWaterTexOffset.y += 0.05f*dt;
	mWaterTexOffset.x += 0.1f*dt;
	DirectX::XMMATRIX wavesOffset = DirectX::XMMatrixTranslation(mWaterTexOffset.x, mWaterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&mWaterTexTransform, wavesScale*wavesOffset);
}

void MyApp::DrawScene()
{
	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Multiply the view and projection matrics
	mCamera.UpdateViewMatrix();
	DirectX::XMMATRIX viewProj = mCamera.ViewProj();

	// Set per frame constants.
	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	ConstBufferPerFrame* cbPerFrame;

	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->dirLight0 = mDirLights[0];
	cbPerFrame->dirLight1 = mDirLights[1];
	cbPerFrame->dirLight2 = mDirLights[2];
	cbPerFrame->eyePosW = mCamera.GetPosition();
	cbPerFrame->fogStart = 15.0f;
	cbPerFrame->fogRange = 175.0f;
	DirectX::XMStoreFloat4(&cbPerFrame->fogColor, Colors::Silver);
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	ConstBufferPerObject* cbPerObject;

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw Box
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mBoxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMMatrixIdentity());
		cbPerObject->material = mBoxMat;
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetInputLayout(mVertexLayout);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mBoxIB, DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->VSSetShader(mVertexShader, NULL, 0);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mNoCullRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, &mBoxMapSRV);
		mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);
		mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

		// Set Output Merger Stage
		mImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);

		// Draw Object
		mImmediateContext->DrawIndexed(36, 0, 0);
	}

	// Draw Hills
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mLandWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mGrassTexTransform));
		cbPerObject->material = mLandMat;
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, &mLandVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mLandIB, DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(0);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, &mGrassMapSRV);

		// Draw Object
		mImmediateContext->DrawIndexed(mLandIndexCount, 0, 0);
	}

	// Draw Waves
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mWavesWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mWaterTexTransform));
		cbPerObject->material = mWavesMat;
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, &mWavesVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mWavesIB, DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mWireframeRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, &mWavesMapSRV);

		// Set Output Merger Stage
		mImmediateContext->OMSetBlendState(mBlendState, blendFactor, 0xffffffff);

		// Draw Object
		mImmediateContext->DrawIndexed(3 * mWaves.TriangleCount(), 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}