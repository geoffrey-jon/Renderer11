/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance), 
	mVertexBuffer(0), 
	mIndexBuffer(0), 
	mInputLayout(0), 
	mWireframeRS(0),
	mTheta(1.5f*MathHelper::Pi), 
	mPhi(0.1f*MathHelper::Pi), 
	mRadius(15.0f),
	mVertexShader(0),
	mPixelShader(0),
	mConstBuffer(0)
{
	mWindowTitle = L"Shapes Demo";
}

MyApp::~MyApp()
{
	ReleaseCOM(mVertexBuffer);
	ReleaseCOM(mIndexBuffer);
	ReleaseCOM(mInputLayout);
	ReleaseCOM(mWireframeRS);
	ReleaseCOM(mConstBuffer);
	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
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
	BuildGeometryBuffers();
	PositionObjects();

	// Compile Shaders
	BuildShaders();

	// Construct and Bind the Rasterizer State
	BuildRasterizerState();

	return true;
}

void MyApp::InitUserInput()
{
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;
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

	mImmediateContext->RSSetState(mWireframeRS);

	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mImmediateContext->IASetIndexBuffer(mIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	// Multiply the view and projection matrics
	DirectX::XMMATRIX view = XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&mProj);
	DirectX::XMMATRIX viewProj = view*proj;

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	ConstBuffer* dataPtr;

	// Draw non-instanced geometry
	mImmediateContext->IASetInputLayout(mInputLayout);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	mImmediateContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);
	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);

	// Draw the grid
	DirectX::XMMATRIX world = XMLoadFloat4x4(&mGridWorld);

	mImmediateContext->Map(mConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (ConstBuffer*)mappedResource.pData;
	dataPtr->worldViewProj = DirectX::XMMatrixTranspose(world*viewProj);
	mImmediateContext->Unmap(mConstBuffer, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBuffer);
	mImmediateContext->DrawIndexed(mGridIndexCount, mGridIndexOffset, mGridVertexOffset);

	// Draw the box.
	world = XMLoadFloat4x4(&mBoxWorld);

	mImmediateContext->Map(mConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (ConstBuffer*)mappedResource.pData;
	dataPtr->worldViewProj = DirectX::XMMatrixTranspose(world*viewProj);
	mImmediateContext->Unmap(mConstBuffer, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBuffer);
	mImmediateContext->DrawIndexed(mBoxIndexCount, mBoxIndexOffset, mBoxVertexOffset);

	// Draw center sphere.
	world = XMLoadFloat4x4(&mCenterSphere);

	mImmediateContext->Map(mConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (ConstBuffer*)mappedResource.pData;
	dataPtr->worldViewProj = DirectX::XMMatrixTranspose(world*viewProj);
	mImmediateContext->Unmap(mConstBuffer, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBuffer);
	mImmediateContext->DrawIndexed(mSphereIndexCount, mSphereIndexOffset, mSphereVertexOffset);
	
	// Draw the instanced geometry
	mImmediateContext->IASetInputLayout(mInstancedInputLayout);

	UINT strides[2] = { sizeof(Vertex), sizeof(InstancedVertex) };
	UINT offsets[2] = { 0, 0 };
	ID3D11Buffer* vBuffers[2] = {mVertexBuffer, mInstanceBuffer};
	mImmediateContext->IASetVertexBuffers(0, 2, vBuffers, strides, offsets);

	mImmediateContext->VSSetShader(mInstancedVertexShader, NULL, 0);

	mImmediateContext->Map(mConstBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	dataPtr = (ConstBuffer*)mappedResource.pData;
	dataPtr->worldViewProj = DirectX::XMMatrixTranspose(viewProj);
	mImmediateContext->Unmap(mConstBuffer, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBuffer);

	// Draw the cylinders.
	mImmediateContext->DrawIndexedInstanced(mCylinderIndexCount, 10, mCylinderIndexOffset, mCylinderVertexOffset, 0);

	// Draw the spheres.
	mImmediateContext->DrawIndexedInstanced(mSphereIndexCount, 10, mSphereIndexOffset, mSphereVertexOffset, 10);

	HR(mSwapChain->Present(0, 0));
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

void MyApp::BuildGeometryBuffers()
{
	GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

	// Cache the vertex offsets to each object in the concatenated vertex buffer.
	mBoxVertexOffset = 0;
	mGridVertexOffset = box.Vertices.size();
	mSphereVertexOffset = mGridVertexOffset + grid.Vertices.size();
	mCylinderVertexOffset = mSphereVertexOffset + sphere.Vertices.size();

	// Cache the index count of each object.
	mBoxIndexCount = box.Indices.size();
	mGridIndexCount = grid.Indices.size();
	mSphereIndexCount = sphere.Indices.size();
	mCylinderIndexCount = cylinder.Indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	mBoxIndexOffset = 0;
	mGridIndexOffset = mBoxIndexCount;
	mSphereIndexOffset = mGridIndexOffset + mGridIndexCount;
	mCylinderIndexOffset = mSphereIndexOffset + mSphereIndexCount;

	UINT totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	UINT totalIndexCount =
		mBoxIndexCount +
		mGridIndexCount +
		mSphereIndexCount +
		mCylinderIndexCount;

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<Vertex> vertices(totalVertexCount);
	std::vector<InstancedVertex> instancedVertices(20);

	DirectX::XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Color = black;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = black;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = black;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = black;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mVertexBuffer));

	for (size_t i = 0; i < 5; ++i)
	{
		DirectX::XMMATRIX m1 = DirectX::XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f);
		DirectX::XMMATRIX m2 = DirectX::XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f);

		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 0].TexCoord0, m1.r[0]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 0].TexCoord1, m1.r[1]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 0].TexCoord2, m1.r[2]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 0].TexCoord3, m1.r[3]);

		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 1].TexCoord0, m2.r[0]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 1].TexCoord1, m2.r[1]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 1].TexCoord2, m2.r[2]);
		DirectX::XMStoreFloat4(&instancedVertices[i * 2 + 1].TexCoord3, m2.r[3]);
	}

	for (size_t i = 0; i < 5; ++i)
	{
		DirectX::XMMATRIX m1 = DirectX::XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f);
		DirectX::XMMATRIX m2 = DirectX::XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f);

		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 0].TexCoord0, m1.r[0]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 0].TexCoord1, m1.r[1]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 0].TexCoord2, m1.r[2]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 0].TexCoord3, m1.r[3]);

		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 1].TexCoord0, m2.r[0]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 1].TexCoord1, m2.r[1]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 1].TexCoord2, m2.r[2]);
		DirectX::XMStoreFloat4(&instancedVertices[(i + 5) * 2 + 1].TexCoord3, m2.r[3]);
	}

	D3D11_BUFFER_DESC ivbd;
	ivbd.Usage = D3D11_USAGE_IMMUTABLE;
	ivbd.ByteWidth = sizeof(InstancedVertex) * 20;
	ivbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ivbd.CPUAccessFlags = 0;
	ivbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA ivinitData;
	ivinitData.pSysMem = &instancedVertices[0];
	HR(mDevice->CreateBuffer(&ivbd, &ivinitData, &mInstanceBuffer));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	std::vector<UINT> indices;
	indices.insert(indices.end(), box.Indices.begin(), box.Indices.end());
	indices.insert(indices.end(), grid.Indices.begin(), grid.Indices.end());
	indices.insert(indices.end(), sphere.Indices.begin(), sphere.Indices.end());
	indices.insert(indices.end(), cylinder.Indices.begin(), cylinder.Indices.end());

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * totalIndexCount;
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mIndexBuffer));
}

void MyApp::PositionObjects()
{
	DirectX::XMMATRIX I = DirectX::XMMatrixIdentity();
	XMStoreFloat4x4(&mGridWorld, I);

	DirectX::XMMATRIX boxScale = DirectX::XMMatrixScaling(2.0f, 1.0f, 2.0f);
	DirectX::XMMATRIX boxOffset = DirectX::XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&mBoxWorld, XMMatrixMultiply(boxScale, boxOffset));

	DirectX::XMMATRIX centerSphereScale = DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f);
	DirectX::XMMATRIX centerSphereOffset = DirectX::XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&mCenterSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));
}

void MyApp::BuildShaders()
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(L"Shaders/VertexShader.hlsl", 0, 0, "VS", "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, &mVertexShader));

	ID3DBlob* InstVSByteCode = 0;
	HR(D3DCompileFromFile(L"Shaders/InstancedVertexShader.hlsl", 0, 0, "VS", "vs_5_0", D3DCOMPILE_DEBUG, 0, &InstVSByteCode, 0));

	HR(mDevice->CreateVertexShader(InstVSByteCode->GetBufferPointer(), InstVSByteCode->GetBufferSize(), NULL, &mInstancedVertexShader));

	ID3DBlob* PSByteCode = 0;
	HR(D3DCompileFromFile(L"Shaders/PixelShader.hlsl", 0, 0, "PS", "ps_5_0", D3DCOMPILE_DEBUG, 0, &PSByteCode, 0));

	HR(mDevice->CreatePixelShader(PSByteCode->GetBufferPointer(), PSByteCode->GetBufferSize(), NULL, &mPixelShader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, 2, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mInputLayout));

	D3D11_INPUT_ELEMENT_DESC instancedVertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
	};

	// Create the input layout
	HR(mDevice->CreateInputLayout(instancedVertexDesc, 6, InstVSByteCode->GetBufferPointer(), InstVSByteCode->GetBufferSize(), &mInstancedInputLayout));

	VSByteCode->Release();
	InstVSByteCode->Release();
	PSByteCode->Release();

	D3D11_BUFFER_DESC matrixBufferDesc;

	matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(ConstBuffer);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;

	HR(mDevice->CreateBuffer(&matrixBufferDesc, NULL, &mConstBuffer));
}

void MyApp::BuildRasterizerState()
{
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

	HR(mDevice->CreateRasterizerState(&wireframeDesc, &mWireframeRS));
}
