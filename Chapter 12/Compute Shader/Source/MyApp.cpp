/*  ======================
	Summary: Blending Demo
	======================  */

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
	mHBlurCS(0),
	mVBlurCS(0),
	mOffscreenRTV(0),
	mOffscreenSRV(0),
	mOffscreenUAV(0),
	mBlurSRV(0),
	mBlurUAV(0),
	mRenderTextureVS(0),
	mRenderTexturePS(0),
	mScreenQuadVB(0),
	mScreenQuadIB(0),
	mConstBufferFullscreen(0)
{
	mWindowTitle = L"Compute Shader Blur Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexLayout);

	ReleaseCOM(mNoCullRS);
	ReleaseCOM(mWireframeRS);
	ReleaseCOM(mSamplerState);
	ReleaseCOM(mBlendState);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	RenderStates::InitAll(mDevice);

	// Initialize Camera
	mCamera.SetPosition(30.0f, 50.0f, -100.0f);
	mCamera.RotateY(-MathHelper::Pi / 8.0f);
	mCamera.Pitch(MathHelper::Pi / 8.0f);

	// Initialize User Input
	InitUserInput();

	// Create Objects
	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject);

	mHillObject = new GHill();
	CreateGeometryBuffers(mHillObject);

	mWaveObject = new GWave();
	CreateGeometryBuffers(mWaveObject, true);

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateVertexShader(&mRenderTextureVS, L"Shaders/RenderTextureVS.hlsl", "VS");
	CreatePixelShader(&mRenderTexturePS, L"Shaders/RenderTexturePS.hlsl", "PS");

	CreateComputeShader(&mHBlurCS, L"Shaders/HBlurCS.hlsl", "CS");
	CreateComputeShader(&mVBlurCS, L"Shaders/VBlurCS.hlsl", "CS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferFullscreen, sizeof(ConstBufferFullscreen));

	// Construct and Bind the Rasterizer State
	CreateRasterizerState();
	CreateSamplerState();
	CreateBlendState();

	SetupStaticLights();

	BuildFullScreenQuad();
	BuildOffscreenViews();
	BuildBlurViews();

	numBlurs = 0;

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

void MyApp::CreateGeometryBuffers(GObject* obj, bool dynamic)
{
	D3D11_BUFFER_DESC vbd;
	if (dynamic) 
	{
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else 
	{
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.CPUAccessFlags = 0;
	}
	vbd.ByteWidth = sizeof(Vertex) * obj->GetVertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
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

void MyApp::PositionObjects()
{
	mWaveObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mWaveObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f));
	mWaveObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f));

	mHillObject->SetTextureScaling(10.0f, 10.0f);
	mHillObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mHillObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mHillObject->SetSpecular(DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f));

	mBoxObject->Translate(8.0f, 5.0f, -15.0f);
	mBoxObject->Rotate(0.0f, 0.0f, 0.0f);
	mBoxObject->Scale(15.0f, 15.0f, 15.0f);

	mBoxObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mBoxObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	LoadTextureToSRV(mWaveObject->GetDiffuseMapSRV(), L"Textures/water2.dds");
	LoadTextureToSRV(mHillObject->GetDiffuseMapSRV(), L"Textures/grass.dds");
	LoadTextureToSRV(mBoxObject->GetDiffuseMapSRV(), L"Textures/wirefence.dds");
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

void MyApp::CreateComputeShader(ID3D11ComputeShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* CSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "cs_5_0", D3DCOMPILE_DEBUG, 0, &CSByteCode, 0));

	HR(mDevice->CreateComputeShader(CSByteCode->GetBufferPointer(), CSByteCode->GetBufferSize(), NULL, shader));

	CSByteCode->Release();
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
	BuildOffscreenViews();
	BuildBlurViews();
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

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(mImmediateContext->Map(*mWaveObject->GetVertexBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	mWaveObject->Update(mTimer.TotalTime(), dt, mappedData.pData);

	mImmediateContext->Unmap(*mWaveObject->GetVertexBuffer(), 0);
}

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x31)
	{
		numBlurs = max(numBlurs - 1, 0);
	}
	else if (key == 0x32)
	{
		numBlurs++;
	}
}

void MyApp::BuildFullScreenQuad()
{
	Vertex v[4];

	v[0].Pos = DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f);
	v[1].Pos = DirectX::XMFLOAT3(-1.0f, +1.0f, 0.0f);
	v[2].Pos = DirectX::XMFLOAT3(+1.0f, +1.0f, 0.0f);
	v[3].Pos = DirectX::XMFLOAT3(+1.0f, -1.0f, 0.0f);

	// Store far plane frustum corner indices in Normal.x slot.
	v[0].Normal = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	v[1].Normal = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
	v[2].Normal = DirectX::XMFLOAT3(2.0f, 0.0f, 0.0f);
	v[3].Normal = DirectX::XMFLOAT3(3.0f, 0.0f, 0.0f);

	v[0].Tex = DirectX::XMFLOAT2(0.0f, 1.0f);
	v[1].Tex = DirectX::XMFLOAT2(0.0f, 0.0f);
	v[2].Tex = DirectX::XMFLOAT2(1.0f, 0.0f);
	v[3].Tex = DirectX::XMFLOAT2(1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * 4;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = v;

	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB));

	USHORT indices[6] =
	{
		0, 1, 2,
		0, 2, 3
	};

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * 6;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.StructureByteStride = 0;
	ibd.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = indices;

	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB));
}

void MyApp::BuildOffscreenViews()
{
	// We call this function everytime the window is resized so that the render target is a quarter
	// the client area dimensions.  So Release the previous views before we create new ones.
	ReleaseCOM(mOffscreenSRV);
	ReleaseCOM(mOffscreenRTV);
	ReleaseCOM(mOffscreenUAV);

	D3D11_TEXTURE2D_DESC texDesc;

	texDesc.Width = mClientWidth;
	texDesc.Height = mClientHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	ID3D11Texture2D* offscreenTex = 0;
	HR(mDevice->CreateTexture2D(&texDesc, 0, &offscreenTex));

	// Null description means to create a view to all mipmap levels using 
	// the format the texture was created with.
	HR(mDevice->CreateShaderResourceView(offscreenTex, 0, &mOffscreenSRV));
	HR(mDevice->CreateRenderTargetView(offscreenTex, 0, &mOffscreenRTV));
	HR(mDevice->CreateUnorderedAccessView(offscreenTex, 0, &mOffscreenUAV));

	// View saves a reference to the texture so we can release our reference.
	ReleaseCOM(offscreenTex);
}

void MyApp::BuildBlurViews()
{
	// Start fresh.
	ReleaseCOM(mBlurSRV);
	ReleaseCOM(mBlurUAV);

	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D11_TEXTURE2D_DESC blurredTexDesc;
	blurredTexDesc.Width = mClientWidth;
	blurredTexDesc.Height = mClientHeight;
	blurredTexDesc.MipLevels = 1;
	blurredTexDesc.ArraySize = 1;
	blurredTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	blurredTexDesc.SampleDesc.Count = 1;
	blurredTexDesc.SampleDesc.Quality = 0;
	blurredTexDesc.Usage = D3D11_USAGE_DEFAULT;
	blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	blurredTexDesc.CPUAccessFlags = 0;
	blurredTexDesc.MiscFlags = 0;

	ID3D11Texture2D* blurredTex = 0;
	HR(mDevice->CreateTexture2D(&blurredTexDesc, 0, &blurredTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	HR(mDevice->CreateShaderResourceView(blurredTex, &srvDesc, &mBlurSRV));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	HR(mDevice->CreateUnorderedAccessView(blurredTex, &uavDesc, &mBlurUAV));

	// Views save a reference to the texture so we can release our reference.
	ReleaseCOM(blurredTex);
}

void MyApp::DrawGeometry()
{
	mImmediateContext->OMSetRenderTargets(1, &mOffscreenRTV, mDepthStencilView);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mOffscreenRTV, reinterpret_cast<const float*>(&Colors::Silver));
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
		world = XMLoadFloat4x4(&mBoxObject->GetWorldTransform());
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mBoxObject->GetTexTransform()));
		cbPerObject->material = mBoxObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetInputLayout(mVertexLayout);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->VSSetShader(mVertexShader, NULL, 0);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mNoCullRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, mBoxObject->GetDiffuseMapSRV());
		mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);
		mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

		// Set Output Merger Stage
		mImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);

		// Draw Object
		mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);
	}

	// Draw Hills
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mHillObject->GetWorldTransform());
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mHillObject->GetTexTransform()));
		cbPerObject->material = mHillObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, mHillObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mHillObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(0);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, mHillObject->GetDiffuseMapSRV());

		// Draw Object
		mImmediateContext->DrawIndexed(mHillObject->GetIndexCount(), 0, 0);
	}

	// Draw Waves
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mWaveObject->GetWorldTransform());
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mWaveObject->GetTexTransform()));
		cbPerObject->material = mWaveObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, mWaveObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mWaveObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mWireframeRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, mWaveObject->GetDiffuseMapSRV());

		// Set Output Merger Stage
		mImmediateContext->OMSetBlendState(mBlendState, blendFactor, 0xffffffff);

		// Draw Object
		mImmediateContext->DrawIndexed(mWaveObject->GetIndexCount(), 0, 0);
	}

	mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);
}

void MyApp::BlurPass()
{
	for (int i = 0; i < numBlurs; ++i)
	{
		// Horizontal Blur
		mImmediateContext->CSSetShaderResources(0, 1, &mOffscreenSRV);
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, &mBlurUAV, 0);

		mImmediateContext->CSSetShader(mHBlurCS, NULL, 0);

		UINT threadGroupsX = (UINT)ceil(mClientWidth / 256.0f);
		mImmediateContext->Dispatch(threadGroupsX, mClientHeight, 1);

		// Clean-up
		ID3D11ShaderResourceView* nullSRV[1] = { NULL };
		mImmediateContext->CSSetShaderResources(0, 1, nullSRV);

		ID3D11UnorderedAccessView* nullUAV[1] = { NULL };
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

		// Vertical Blur
		mImmediateContext->CSSetShaderResources(0, 1, &mBlurSRV);
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, &mOffscreenUAV, 0);

		mImmediateContext->CSSetShader(mVBlurCS, NULL, 0);

		UINT threadGroupsY = (UINT)ceil(mClientHeight / 256.0f);
		mImmediateContext->Dispatch(mClientWidth, threadGroupsY, 1);

		// Clean-up
		mImmediateContext->CSSetShaderResources(0, 1, nullSRV);
		mImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, 0);

		mImmediateContext->CSSetShader(NULL, NULL, 0);
	}
}

void MyApp::DrawFinal()
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);

	// Scale and shift quad to lower-right corner.
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	mImmediateContext->Map(mConstBufferFullscreen, 0, D3D11_MAP_WRITE_DISCARD, 0, &mConstBufferFullscreenResource);
	cbFullscreen = (ConstBufferFullscreen*)mConstBufferFullscreenResource.pData;
	cbFullscreen->world = DirectX::XMMatrixTranspose(world);
	mImmediateContext->Unmap(mConstBufferFullscreen, 0);

	mImmediateContext->VSSetShader(mRenderTextureVS, NULL, 0);
	mImmediateContext->PSSetShader(mRenderTexturePS, NULL, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferFullscreen);

	mImmediateContext->PSSetShaderResources(0, 1, &mOffscreenSRV);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);

	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	mImmediateContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* nullSRV = { 0 };
	mImmediateContext->PSSetShaderResources(0, 1, &nullSRV);
}

void MyApp::DrawScene()
{
	DrawGeometry();
	BlurPass();
	DrawFinal();

	HR(mSwapChain->Present(0, 0));
}