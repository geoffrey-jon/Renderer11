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
	mWireframeRS(0)
{
	mWindowTitle = L"First Person Camera Demo";
}

MyApp::~MyApp()
{
	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexLayout);
	ReleaseCOM(mWireframeRS);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initialize Camera
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	// Initialize User Input
	InitUserInput();

	// Create the geometry for the demo and set their world positions
	mSkullObject = new GObject("Models/skull.txt");
	BuildGeometryBuffers(mSkullObject);

	mBoxObject = new GCube();
	BuildGeometryBuffers(mBoxObject);

	mPlaneObject = new GPlane();
	BuildGeometryBuffers(mPlaneObject);

	PositionObjects();

	// Compile Shaders
	BuildVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	BuildPixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));

	// Construct and Bind the Rasterizer State
	BuildRasterizerState();
	BuildSamplerState();

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

void MyApp::BuildGeometryBuffers(GObject* obj)
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

void MyApp::PositionObjects()
{
	mPlaneObject->SetAmbient(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));
	mPlaneObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mPlaneObject->SetSpecular(DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f));

	LoadTextureToSRV(mPlaneObject->GetDiffuseMapSRV(), L"Textures/WoodCrate01.dds");

	mBoxObject->Translate(0.0f, 0.5f, 0.0f);
	mBoxObject->Rotate(0.0f, 0.0f, 0.0f);
	mBoxObject->Scale(3.0f, 1.0f, 3.0f);

	mBoxObject->SetAmbient(DirectX::XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f));
	mBoxObject->SetDiffuse(DirectX::XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f));
	mBoxObject->SetSpecular(DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f));

	LoadTextureToSRV(mBoxObject->GetDiffuseMapSRV(), L"Textures/WoodCrate02.dds");

	mSkullObject->Translate(0.0f, 1.0f, 0.0f);
	mSkullObject->Rotate(0.0f, 0.0f, 0.0f);
	mSkullObject->Scale(0.5f, 0.5f, 0.5f);

	mSkullObject->SetAmbient(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mSkullObject->SetDiffuse(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mSkullObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
}

void MyApp::BuildVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
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

void MyApp::BuildPixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint)
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

void MyApp::BuildRasterizerState()
{
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_SOLID;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(mDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));
}

void MyApp::BuildSamplerState()
{
	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 1;
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
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
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
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
	ConstBufferPerObject* cbPerObject;

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw Plane
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mPlaneObject->GetWorldTransform());
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(XMLoadFloat4x4(&mPlaneObject->GetTexTransform()));
		cbPerObject->material = mPlaneObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetInputLayout(mVertexLayout);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mImmediateContext->IASetVertexBuffers(0, 1, mPlaneObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mPlaneObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->VSSetShader(mVertexShader, NULL, 0);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mWireframeRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, mPlaneObject->GetDiffuseMapSRV());
		mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);
		mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

		// Draw Object
		mImmediateContext->DrawIndexed(mPlaneObject->GetIndexCount(), 0, 0);
	}

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
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(XMLoadFloat4x4(&mBoxObject->GetTexTransform()));
		cbPerObject->material = mBoxObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, mBoxObject->GetDiffuseMapSRV());
		mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);

		// Draw Object
		mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);
	}

	// Draw Skull
	{
		// Compute object matrices
		world = XMLoadFloat4x4(&mSkullObject->GetWorldTransform());
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*viewProj;

		// Set per object constants
		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(XMLoadFloat4x4(&mPlaneObject->GetTexTransform()));
		cbPerObject->material = mSkullObject->GetMaterial();
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetVertexBuffers(0, 1, mSkullObject->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mSkullObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Set Rasterizer Stage

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		// Draw Object
		mImmediateContext->DrawIndexed(mSkullObject->GetIndexCount(), 0, 0);
	}

	HR(mSwapChain->Present(0, 0));
}