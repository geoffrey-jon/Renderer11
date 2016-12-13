/*  ======================
	Summary: Picking Demo
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
	mVertexLayout(0),
	bPicked(false),
	mLessEqualDSS(0)
{
	mWindowTitle = L"Picking Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);
	ReleaseCOM(mVertexLayout);

	ReleaseCOM(mLessEqualDSS);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initialize Camera
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	// Initialize User Input
	InitUserInput();

	// Create Objects
	mCarObject = new GObject("Models/car.txt");
	CreateIndexedGeometryBuffers(mCarObject);

	mPickedTriangle = new GTriangle();
	CreateGeometryBuffers(mPickedTriangle, true);

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShaderNoTexture, L"Shaders/PixelShaderNoTexture.hlsl", "PS");

	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
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
	if ((btnState & MK_LBUTTON) != 0)
	{
		mLastMousePos.x = x;
		mLastMousePos.y = y;

		SetCapture(mMainWindow);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		Pick(x, y);
	}
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
}

void MyApp::CreateIndexedGeometryBuffers(GObject* obj, bool dynamic)
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
	mPickedTriangle->Translate(0.0f, 1.0f, 0.0f);
	mPickedTriangle->Scale(0.5f, 0.5f, 0.5f);

	mPickedTriangle->SetAmbient(DirectX::XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f));
	mPickedTriangle->SetDiffuse(DirectX::XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f));
	mPickedTriangle->SetSpecular(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f));

	mCarObject->Translate(0.0f, 1.0f, 0.0f);
	mCarObject->Scale(0.5f, 0.5f, 0.5f);

	mCarObject->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
	mCarObject->SetDiffuse(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mCarObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
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
}

void MyApp::InitBlendStates()
{
}

void MyApp::InitDepthStencilStates()
{
	// LessEqualDSS
	D3D11_DEPTH_STENCIL_DESC lessEqualDesc;
	lessEqualDesc.DepthEnable = true;
	lessEqualDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	lessEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	lessEqualDesc.StencilEnable = false;

	HR(mDevice->CreateDepthStencilState(&lessEqualDesc, &mLessEqualDSS));
}

void MyApp::InitSamplerStates()
{
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
	{
		mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));
		mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		mImmediateContext->IASetInputLayout(mVertexLayout);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Update Camera
	mCamera.UpdateViewMatrix();
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Set per frame constants
	{
		mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
		cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
		cbPerFrame->dirLight0 = mDirLights[0];
		cbPerFrame->dirLight1 = mDirLights[1];
		cbPerFrame->dirLight2 = mDirLights[2];
		cbPerFrame->eyePosW = mCamera.GetPosition();
		mImmediateContext->Unmap(mConstBufferPerFrame, 0);
	}

	// Bind Constant Buffers to the Pipeline
	{
		mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
		mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
	}

	// Draw Car
	{
		mImmediateContext->RSSetState(0);
		mImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
		mImmediateContext->OMSetDepthStencilState(0, 0);

		mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
		mImmediateContext->PSSetShader(mPixelShaderNoTexture, NULL, 0);

		DrawObjectIndexed(mCarObject);
	}

	if (bPicked == true)
	{
		mImmediateContext->OMSetDepthStencilState(mLessEqualDSS, 0);
		DrawObject(mPickedTriangle);
	}

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

	if (object->GetDiffuseMapSRV())
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

	// Draw Object
	mImmediateContext->Draw(object->GetVertexCount(), 0);
}

void MyApp::DrawObjectIndexed(GObject* object)
{
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldInvTranspose;
	DirectX::XMMATRIX worldViewProj;

	// Compute object matrices
	world = XMLoadFloat4x4(&object->GetWorldTransform());
	worldInvTranspose = MathHelper::InverseTranspose(world);
	worldViewProj = world*mCamera.ViewProj();

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

	if (object->GetDiffuseMapSRV())
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

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

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&object->GetTexTransform()));
//	cbPerObject->material = mShadowMat;
	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Input Assembler Stage
	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	if (object->GetDiffuseMapSRV())
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

	// Draw Object
	mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
}

void MyApp::Pick(int sx, int sy)
{
	DirectX::XMFLOAT4X4 P;
	DirectX::XMStoreFloat4x4(&P, mCamera.Proj());

	// Compute picking ray in view space.
	float vx = (+2.0f*sx / mClientWidth - 1.0f) / P.m[0][0];
	float vy = (-2.0f*sy / mClientHeight + 1.0f) / P.m[1][1];

	// Ray definition in view space.
	DirectX::XMVECTOR rayOrigin = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMVECTOR rayDir = DirectX::XMVectorSet(vx, vy, 1.0f, 0.0f);

	// Tranform ray to local space of Mesh.
	DirectX::XMMATRIX V = mCamera.View();
	DirectX::XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	bPicked = mCarObject->Pick(rayOrigin, rayDir, invView, mPickedTriangle);

	if (bPicked == true)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HR(mImmediateContext->Map(*mPickedTriangle->GetVertexBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

		mPickedTriangle->Update(mappedData.pData);

		mImmediateContext->Unmap(*mPickedTriangle->GetVertexBuffer(), 0);
	}
}
