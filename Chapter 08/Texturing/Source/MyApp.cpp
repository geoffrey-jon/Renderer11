/*  =======================
Summary: DX11 Chapter 4
=======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance),
	mBoxVertexBuffer(0),
	mBoxIndexBuffer(0),
	mDiffuseMapSRV(0),
	mConstBufferPerFrame(0),
	mConstBufferPerObject(0),
	mVertexShader(0),
	mPixelShader(0),
	mLightCount(1),
	mEyePosW(0.0f, 0.0f, 0.0f),
	mTheta(1.3f*MathHelper::Pi),
	mPhi(0.4f*MathHelper::Pi),
	mRadius(2.5f),
	mInputLayout(0),
	mWireframeRS(0)
{
	mWindowTitle = L"Crate Demo";
}

MyApp::~MyApp()
{
	ReleaseCOM(mBoxVertexBuffer);
	ReleaseCOM(mBoxIndexBuffer);
	ReleaseCOM(mDiffuseMapSRV);
	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
}

bool MyApp::Init()
{
	// Initialize View and Projection Matrices
	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mView, I);
	XMStoreFloat4x4(&mProj, I);

	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initialize User Input
	InitUserInput();

	// Create the geometry for the demo and set their world positions
	BuildShapeGeometryBuffers();
	PositionObjects();

	// Compile Shaders
	BuildVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	BuildPixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));

	// Construct and Bind the Rasterizer State
	BuildRasterizerState();
	BuildSamplerState();
	LoadTextureToSRV(L"Textures/WoodCrate02.dds", &mDiffuseMapSRV);
	SetupStaticLights();

	return true;
}

void MyApp::LoadTextureToSRV(LPCWSTR filename, ID3D11ShaderResourceView** SRV)
{
	ID3D11Resource* texResource = nullptr;
	HR(DirectX::CreateDDSTextureFromFile(mDevice, filename, &texResource, SRV));
	ReleaseCOM(texResource); // view saves reference
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

		// Update angles based on input to orbit camera around box.
		mTheta += dx;
		mPhi += dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.01 unit in the scene.
		float dx = 0.01f*static_cast<float>(x - mLastMousePos.x);
		float dy = 0.01f*static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 200.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void MyApp::BuildShapeGeometryBuffers()
{
	GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;

	// Cache the index count of each object.
	mBoxIndexCount = box.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;

	UINT totalVertexCount = box.Vertices.size();

	UINT totalIndexCount = mBoxIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].Tex = box.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mBoxVertexBuffer));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mBoxIndexBuffer));
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
	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mBoxWorld, I);
	XMStoreFloat4x4(&mTexTransform, I);

	mBoxMat.Ambient = DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mBoxMat.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mBoxMat.Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
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
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mInputLayout));

	VSByteCode->Release();
}

void MyApp::BuildPixelShader(ID3D11PixelShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* PSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ps_5_0", D3DCOMPILE_DEBUG, 0, &PSByteCode, 0));

	HR(mDevice->CreatePixelShader(PSByteCode->GetBufferPointer(), PSByteCode->GetBufferSize(), NULL, shader));

	PSByteCode->Release();
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
	mDirLights[0].Ambient = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	mDirLights[0].Diffuse = DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	mDirLights[0].Specular = DirectX::XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
	mDirLights[0].Direction = DirectX::XMFLOAT3(0.707f, -0.707f, 0.0f);

	mDirLights[1].Ambient = DirectX::XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	mDirLights[1].Diffuse = DirectX::XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
	mDirLights[1].Specular = DirectX::XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
	mDirLights[1].Direction = DirectX::XMFLOAT3(-0.707f, 0.0f, 0.707f);
}

void MyApp::OnResize()
{
	D3DApp::OnResize();

	// Update the Projection matrix based on the window size
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void MyApp::UpdateScene(float dt)
{
	// Convert Spherical to Cartesian coordinates.
	float x = mRadius*sinf(mPhi)*cosf(mTheta);
	float z = mRadius*sinf(mPhi)*sinf(mTheta);
	float y = mRadius*cosf(mPhi);

	mEyePosW = DirectX::XMFLOAT3(x, y, z);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(x, y, z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	// Update the view matrix
	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, V);
}

void MyApp::DrawScene()
{
	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Multiply the view and projection matrics
	DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	// Set per frame constants
	D3D11_MAPPED_SUBRESOURCE cbPerFrameResource;
	ConstBufferPerFrame* cbPerFrame;

	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->dirLight0 = mDirLights[0];
	cbPerFrame->dirLight1 = mDirLights[1];
	cbPerFrame->eyePosW = mEyePosW;
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	// Draw Textured Box
	{
		DirectX::XMMATRIX world = XMLoadFloat4x4(&mBoxWorld);
		DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		DirectX::XMMATRIX worldViewProj = world*viewProj;
		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		// Set per object constants
		D3D11_MAPPED_SUBRESOURCE cbPerObjectResource;
		ConstBufferPerObject* cbPerObject;

		mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
		cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
		cbPerObject->world = DirectX::XMMatrixTranspose(world);
		cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
		cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObject->texTransform = DirectX::XMMatrixTranspose(XMLoadFloat4x4(&mTexTransform));
		cbPerObject->material = mBoxMat;
		mImmediateContext->Unmap(mConstBufferPerObject, 0);

		// Set Input Assembler Stage
		mImmediateContext->IASetInputLayout(mInputLayout);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		mImmediateContext->IASetVertexBuffers(0, 1, &mBoxVertexBuffer, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mBoxIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set Vertex Shader Stage
		mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->VSSetShader(mVertexShader, NULL, 0);

		// Set Rasterizer Stage
		mImmediateContext->RSSetState(mWireframeRS);

		// Set Pixel Shader Stage	
		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
		mImmediateContext->PSSetShaderResources(0, 1, &mDiffuseMapSRV);
		mImmediateContext->PSSetSamplers(0, 1, &mSamplerState);
		mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

		mImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);
	} // End Draw Box

	HR(mSwapChain->Present(0, 0));
}
