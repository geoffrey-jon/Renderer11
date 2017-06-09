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
	mBillboardVS(0),
	mBillboardGS(0),
	mBillboardPS(0)
{
	mWindowTitle = L"Geometry Shader Demo";
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

	// Initialize Camera
	mCamera.SetPosition(50.0f, 100.0f, -140.0f);
	mCamera.RotateY(-MathHelper::Pi / 8.0f);
	mCamera.Pitch(MathHelper::Pi / 4.0f);

	// Initialize User Input
	InitUserInput();

	// Create Objects
	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject);

	mHillObject = new GHill();
	CreateGeometryBuffers(mHillObject);

	mWaveObject = new GWave();
	CreateGeometryBuffers(mWaveObject, true);

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(mImmediateContext->Map(*mWaveObject->GetVertexBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	mWaveObject->Update(mTimer.TotalTime(), 0, mappedData.pData);

	mImmediateContext->Unmap(*mWaveObject->GetVertexBuffer(), 0);

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateVertexShaderBillboard(&mBillboardVS, L"Shaders/BillboardVS.hlsl", "VS");
	CreateGeometryShader(&mBillboardGS, L"Shaders/BillboardGS.hlsl", "GS");
	CreatePixelShader(&mBillboardPS, L"Shaders/BillboardPS.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));

	CreateConstantBuffer(&mCBPerFrameBillboard, sizeof(ConstBufferPerFrameBillboard));

	// Construct and Bind the Rasterizer State
	CreateRasterizerState();
	CreateSamplerState();
	CreateBlendState();

	SetupStaticLights();
	BuildBillboardBuffer();

	LoadTextureToSRV(&mTreeTexSRV, L"Textures/tree0.dds");

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

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x31)
	{
		srand(mTimer.TotalTime());
		BuildBillboardBuffer();
	}
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

	mTreeMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mTreeMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTreeMat.Specular = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

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

void MyApp::CreateVertexShaderBillboard(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexBillboard));

	VSByteCode->Release();
}

void MyApp::CreateGeometryShader(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	HR(mDevice->CreateGeometryShader(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), NULL, shader));

	GSByteCode->Release();
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

float MyApp::GetHillHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

void MyApp::BuildBillboardBuffer()
{
	Billboard v[mTreeCount];

	for (UINT i = 0; i < mTreeCount; ++i)
	{
		float x = -1;
		float y = -1;
		float z = -1;
		while (y < 0)
		{
			x = MathHelper::RandF(-75.0f, 75.0f);
			z = MathHelper::RandF(-75.0f, 75.0f);
			y = GetHillHeight(x, z);
		}

		// Move tree slightly above land height.
		y += 10.0f;

		v[i].centerW = DirectX::XMFLOAT3(x, y, z);
		v[i].sizeW = DirectX::XMFLOAT2(24.0f, 24.0f);
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Billboard) * mTreeCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = v;
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mBillboardVB));
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

	mCamera.UpdateViewMatrix();

//	D3D11_MAPPED_SUBRESOURCE mappedData;
//	HR(mImmediateContext->Map(*mWaveObject->GetVertexBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

//	mWaveObject->Update(mTimer.TotalTime(), dt, mappedData.pData);

//	mImmediateContext->Unmap(*mWaveObject->GetVertexBuffer(), 0);
}

void MyApp::DrawGeometry()
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
}

void MyApp::DrawTrees()
{
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	mImmediateContext->IASetInputLayout(mVertexBillboard);

	mImmediateContext->Map(mCBPerFrameBillboard, 0, D3D11_MAP_WRITE_DISCARD, 0, &mCBPerFrameBillboardResource);
	cbPerFrameBillboard = (ConstBufferPerFrameBillboard*)mCBPerFrameBillboardResource.pData;
	cbPerFrameBillboard->dirLight0 = mDirLights[0];
	cbPerFrameBillboard->dirLight1 = mDirLights[1];
	cbPerFrameBillboard->dirLight2 = mDirLights[2];
	cbPerFrameBillboard->viewProj = DirectX::XMMatrixTranspose(mCamera.ViewProj());
	cbPerFrameBillboard->material = mTreeMat;
	cbPerFrameBillboard->eyePosW = mCamera.GetPosition();
	mImmediateContext->Unmap(mCBPerFrameBillboard, 0);

	mImmediateContext->GSSetConstantBuffers(0, 1, &mCBPerFrameBillboard);
	mImmediateContext->PSSetConstantBuffers(0, 1, &mCBPerFrameBillboard);

	mImmediateContext->VSSetShader(mBillboardVS, NULL, 0);
	mImmediateContext->GSSetShader(mBillboardGS, NULL, 0);
	mImmediateContext->PSSetShader(mBillboardPS, NULL, 0);

	mImmediateContext->PSSetShaderResources(0, 1, &mTreeTexSRV);

	UINT stride = sizeof(Billboard);
	UINT offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, &mBillboardVB, &stride, &offset);

	mImmediateContext->Draw(mTreeCount, 0);

	mImmediateContext->GSSetShader(0, NULL, 0);
}

void MyApp::DrawScene()
{
	DrawGeometry();
	DrawTrees();

	HR(mSwapChain->Present(0, 0));
}