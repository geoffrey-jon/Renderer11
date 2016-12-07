/*  ======================
	Summary: Stencil Demo
	======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance),
	mConstBufferPerFrame(0),
	mConstBufferPerObject(0),
	mConstBufferLights(0),
	mVertexShader(0),
	mPixelShader(0),
	mVertexLayout(0),
	mCullClockwiseRS(0),
	mTransparentBS(0),
	mNoRenderTargetWritesBS(0),
	mMarkMirrorDSS(0),
	mDrawReflectionDSS(0),
	mNoDoubleBlendDSS(0),
	mSamplerState(0)
{
	mWindowTitle = L"Stencil Mirror Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mConstBufferLights);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexLayout);

	ReleaseCOM(mCullClockwiseRS);
	ReleaseCOM(mTransparentBS);
	ReleaseCOM(mNoRenderTargetWritesBS);
	ReleaseCOM(mMarkMirrorDSS);
	ReleaseCOM(mDrawReflectionDSS);
	ReleaseCOM(mNoDoubleBlendDSS);
	ReleaseCOM(mSamplerState);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initialize Camera
	mCamera.SetPosition(0.0f, 5.0f, -15.0f);

	// Initialize User Input
	InitUserInput();

	// Create Objects
	mSkullObject = new GObject("Models/skull.txt");
	CreateGeometryBuffers(mSkullObject, true);

	mFloorObject = new GPlaneXZ(12.0f, 10.0f, 2, 2);
	CreateGeometryBuffers(mFloorObject, true);

	mWallPiece1 = new GPlaneXY(2.0f, 6.0f, 2, 2);
	CreateGeometryBuffers(mWallPiece1, true);

	mWallPiece2 = new GPlaneXY(5.0f, 2.0f, 2, 2);
	CreateGeometryBuffers(mWallPiece2, true);

	mWallPiece3 = new GPlaneXY(5.0f, 6.0f, 2, 2);
	CreateGeometryBuffers(mWallPiece3, true);

	mMirrorObject = new GPlaneXY(5.0f, 4.0f, 2, 2);
	CreateGeometryBuffers(mMirrorObject, true);

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");
	CreatePixelShader(&mPixelShaderNoTexture, L"Shaders/PixelShaderNoTexture.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferLights, sizeof(ConstBufferLights));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));

	// Construct and Bind the Rasterizer State
	InitRasterizerStates();
	InitSamplerStates();
	InitBlendStates();
	InitDepthStencilStates();

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
	mShadowMat.Ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mShadowMat.Diffuse = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	mShadowMat.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);

	mFloorObject->Translate(1.5f, 0.0f, -5.0f);
	mFloorObject->SetTextureScaling(4.0, 4.0);

	mFloorObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mFloorObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mFloorObject->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	mWallPiece1->Translate(-3.5f, 3.0f, 0.0f);
	mWallPiece1->SetTextureScaling(1.0, 3.0);

	mWallPiece1->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mWallPiece1->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mWallPiece1->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	mWallPiece2->Translate(0.0f, 5.0f, 0.0f);
	mWallPiece2->SetTextureScaling(2.5, 1.0);

	mWallPiece2->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mWallPiece2->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mWallPiece2->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	mWallPiece3->Translate(5.0f, 3.0f, 0.0f);
	mWallPiece3->SetTextureScaling(2.5, 3.0);

	mWallPiece3->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mWallPiece3->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mWallPiece3->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	mMirrorObject->Translate(0.0f, 2.0f, -0.01f);

	mMirrorObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mMirrorObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f));
	mMirrorObject->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	mSkullObject->Translate(0.0f, 1.0f, -5.0f);
	mSkullObject->Rotate(0.0f, 0.5f*MathHelper::Pi, 0.0f);
	mSkullObject->Scale(0.45f, 0.45f, 0.45f);

	mSkullObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mSkullObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mSkullObject->SetSpecular(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f));

	LoadTextureToSRV(mFloorObject->GetDiffuseMapSRV(), L"Textures/checkboard.dds");
	LoadTextureToSRV(mWallPiece1->GetDiffuseMapSRV(), L"Textures/brick01.dds");
	LoadTextureToSRV(mWallPiece2->GetDiffuseMapSRV(), L"Textures/brick01.dds");
	LoadTextureToSRV(mWallPiece3->GetDiffuseMapSRV(), L"Textures/brick01.dds");
	LoadTextureToSRV(mMirrorObject->GetDiffuseMapSRV(), L"Textures/ice.dds");
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

void MyApp::InitRasterizerStates()
{
	// CullClockwiseRS
	// Note: Define such that we still cull backfaces by making front faces CCW.
	// If we did not cull backfaces, then we have to worry about the BackFace
	// property in the D3D11_DEPTH_STENCIL_DESC.
	D3D11_RASTERIZER_DESC cullClockwiseDesc;
	ZeroMemory(&cullClockwiseDesc, sizeof(D3D11_RASTERIZER_DESC));
	cullClockwiseDesc.FillMode = D3D11_FILL_SOLID;
	cullClockwiseDesc.CullMode = D3D11_CULL_BACK;
	cullClockwiseDesc.FrontCounterClockwise = true;
	cullClockwiseDesc.DepthClipEnable = true;

	HR(mDevice->CreateRasterizerState(&cullClockwiseDesc, &mCullClockwiseRS));
}

void MyApp::InitBlendStates()
{
	// TransparentBS
	D3D11_BLEND_DESC transparentDesc = { 0 };
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	transparentDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	transparentDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	transparentDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(mDevice->CreateBlendState(&transparentDesc, &mTransparentBS));

	// NoRenderTargetWritesBS
	D3D11_BLEND_DESC noRenderTargetWritesDesc = { 0 };
	noRenderTargetWritesDesc.AlphaToCoverageEnable = false;
	noRenderTargetWritesDesc.IndependentBlendEnable = false;

	noRenderTargetWritesDesc.RenderTarget[0].BlendEnable = false;
	noRenderTargetWritesDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	noRenderTargetWritesDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	noRenderTargetWritesDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	noRenderTargetWritesDesc.RenderTarget[0].RenderTargetWriteMask = 0;

	HR(mDevice->CreateBlendState(&noRenderTargetWritesDesc, &mNoRenderTargetWritesBS));
}

void MyApp::InitDepthStencilStates()
{
	// MarkMirrorDSS
	D3D11_DEPTH_STENCIL_DESC mirrorDesc;
	mirrorDesc.DepthEnable = true;
	mirrorDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	mirrorDesc.DepthFunc = D3D11_COMPARISON_LESS;
	mirrorDesc.StencilEnable = true;
	mirrorDesc.StencilReadMask = 0xff;
	mirrorDesc.StencilWriteMask = 0xff;

	mirrorDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// We are not rendering backfacing polygons, so these settings do not matter.
	mirrorDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	mirrorDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HR(mDevice->CreateDepthStencilState(&mirrorDesc, &mMarkMirrorDSS));

	// DrawReflectionDSS
	D3D11_DEPTH_STENCIL_DESC drawReflectionDesc;
	drawReflectionDesc.DepthEnable = true;
	drawReflectionDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	drawReflectionDesc.DepthFunc = D3D11_COMPARISON_LESS;
	drawReflectionDesc.StencilEnable = true;
	drawReflectionDesc.StencilReadMask = 0xff;
	drawReflectionDesc.StencilWriteMask = 0xff;

	drawReflectionDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	drawReflectionDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	drawReflectionDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(mDevice->CreateDepthStencilState(&drawReflectionDesc, &mDrawReflectionDSS));

	// NoDoubleBlendDSS
	D3D11_DEPTH_STENCIL_DESC noDoubleBlendDesc;
	noDoubleBlendDesc.DepthEnable = true;
	noDoubleBlendDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	noDoubleBlendDesc.DepthFunc = D3D11_COMPARISON_LESS;
	noDoubleBlendDesc.StencilEnable = true;
	noDoubleBlendDesc.StencilReadMask = 0xff;
	noDoubleBlendDesc.StencilWriteMask = 0xff;

	noDoubleBlendDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	noDoubleBlendDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	noDoubleBlendDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	noDoubleBlendDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	noDoubleBlendDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	HR(mDevice->CreateDepthStencilState(&noDoubleBlendDesc, &mNoDoubleBlendDSS));
}

void MyApp::InitSamplerStates()
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
}

void MyApp::DrawScene()
{
	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Update Camera
	mCamera.UpdateViewMatrix();

	// Set per frame constants
	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	ConstBufferPerFrame* cbPerFrame;

	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->eyePosW = mCamera.GetPosition();
	cbPerFrame->fogStart = 2.0f;
	cbPerFrame->fogRange = 40.0f;
	DirectX::XMStoreFloat4(&cbPerFrame->fogColor, Colors::Black);
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);

	// Set constant buffer for lights
	D3D11_MAPPED_SUBRESOURCE cbLightsResource;
	ConstBufferLights* cbLights;

	mImmediateContext->Map(mConstBufferLights, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbLightsResource);
	cbLights = (ConstBufferLights*)cbLightsResource.pData;
	cbLights->dirLight0 = mDirLights[0];
	cbLights->dirLight1 = mDirLights[1];
	cbLights->dirLight2 = mDirLights[2];
	mImmediateContext->Unmap(mConstBufferLights, 0);

	mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferLights);
	mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferLights);

	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->RSSetState(0);
	mImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(0, 0);

	// Draw Skull
	mImmediateContext->PSSetShader(mPixelShaderNoTexture, NULL, 0);
	DrawObject(mSkullObject);

	// Draw Floor and Wall Pieces
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);
	DrawObject(mFloorObject);
	DrawObject(mWallPiece1);
	DrawObject(mWallPiece2);
	DrawObject(mWallPiece3);

	// Draw Mirror to Stencil Buffer
	mImmediateContext->OMSetBlendState(mNoRenderTargetWritesBS, blendFactor, 0xffffffff); // Do not write to render target.
	mImmediateContext->OMSetDepthStencilState(mMarkMirrorDSS, 1); // Render visible mirror pixels to stencil buffer. Do not write mirror depth to depth buffer at this point, otherwise it will occlude the reflection.

	DrawObject(mMirrorObject);

	// Draw the skull reflection.
	DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);

	DirectionalLight ReflectedDirLights[3];
	for (int i = 0; i < 3; ++i)
	{
		ReflectedDirLights[i] = mDirLights[i];
		DirectX::XMVECTOR lightDir = XMLoadFloat3(&ReflectedDirLights[i].Direction);
		DirectX::XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&ReflectedDirLights[i].Direction, reflectedLightDir);
	}

	mImmediateContext->Map(mConstBufferLights, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbLightsResource);
	cbLights = (ConstBufferLights*)cbLightsResource.pData;
	cbLights->dirLight0 = ReflectedDirLights[0];
	cbLights->dirLight1 = ReflectedDirLights[1];
	cbLights->dirLight2 = ReflectedDirLights[2];
	mImmediateContext->Unmap(mConstBufferLights, 0);

	mImmediateContext->RSSetState(mCullClockwiseRS); // Cull clockwise triangles for reflection.
	mImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(mDrawReflectionDSS, 1); // Only draw reflection into visible mirror pixels as marked by the stencil buffer. 

	DrawObjectTransform(mSkullObject, R);

	// Draw the mirror to the back buffer as usual but with transparency blending so the reflection shows through.
	mImmediateContext->Map(mConstBufferLights, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbLightsResource);
	cbLights = (ConstBufferLights*)cbLightsResource.pData;
	cbLights->dirLight0 = mDirLights[0];
	cbLights->dirLight1 = mDirLights[1];
	cbLights->dirLight2 = mDirLights[2];
	mImmediateContext->Unmap(mConstBufferLights, 0);

	mImmediateContext->RSSetState(0);
	mImmediateContext->OMSetBlendState(mTransparentBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(0, 0);

	DrawObject(mMirrorObject);

	// Draw the skull shadow.
	mImmediateContext->OMSetDepthStencilState(mNoDoubleBlendDSS, 0);

	DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	DirectX::XMVECTOR toMainLight = DirectX::XMVectorNegate(DirectX::XMLoadFloat3(&mDirLights[0].Direction));
	DirectX::XMMATRIX S = DirectX::XMMatrixShadow(shadowPlane, toMainLight);
	DirectX::XMMATRIX shadowOffsetY = DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f);

	DrawObjectShadow(mSkullObject, S*shadowOffsetY);

	HR(mSwapChain->Present(0, 0));
}

void MyApp::DrawObject(GObject* object)
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;

	// Compute object matrices
	world = XMLoadFloat4x4(&object->GetWorldTransform());
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world*mCamera.ViewProj();

	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	ConstBufferPerObject* cbPerObject;

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&object->GetTexTransform()));
	cbPerObject->material = object->GetMaterial();
	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Input Assembler Stage
	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	// Set Vertex Shader Stage
	mImmediateContext->VSSetConstantBuffers(2, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPerObject);

	if (object->GetDiffuseMapSRV())
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

	// Draw Object
	mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
}

void MyApp::DrawObjectTransform(GObject* object, DirectX::XMMATRIX& tranform)
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;

	// Compute object matrices
	world = XMLoadFloat4x4(&object->GetWorldTransform()) * tranform;
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world*mCamera.ViewProj();

	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	ConstBufferPerObject* cbPerObject;

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&object->GetTexTransform()));
	cbPerObject->material = object->GetMaterial();
	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Input Assembler Stage
	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	// Set Vertex Shader Stage
	mImmediateContext->VSSetConstantBuffers(2, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPerObject);

	// Draw Object
	mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
}

void MyApp::DrawObjectShadow(GObject* object, DirectX::XMMATRIX& tranform)
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;

	// Compute object matrices
	world = XMLoadFloat4x4(&object->GetWorldTransform()) * tranform;
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world*mCamera.ViewProj();

	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	ConstBufferPerObject* cbPerObject;

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&object->GetTexTransform()));
	cbPerObject->material = mShadowMat;
	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Input Assembler Stage
	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	// Set Vertex Shader Stage
	mImmediateContext->VSSetConstantBuffers(2, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPerObject);

	// Draw Object
	mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
}
