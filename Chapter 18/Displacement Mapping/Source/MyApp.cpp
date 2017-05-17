/*  ======================
	Summary: Cube Map Demo
	======================  */

#include "MyApp.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "D3DCompiler.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance),
	mConstBufferPerFrame(0),
	mConstBufferPerObject(0),
	mConstBufferPSParams(0),
	mVertexShader(0),
	mHullShader(0),
	mDomainShader(0),
	mPixelShader(0),
	mSkyVertexShader(0),
	mSkyPixelShader(0),
	mVertexLayout(0),
	mSkullObject(0),
	mFloorObject(0),
	mBoxObject(0),
	mSphereObject(0)
{
	mWindowTitle = L"Displacement Map Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();
	RenderStates::DestroyAll();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mConstBufferPSParams);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mHullShader);
	ReleaseCOM(mDomainShader);
	ReleaseCOM(mPixelShader);

	ReleaseCOM(mSkyVertexShader);
	ReleaseCOM(mSkyPixelShader);

	ReleaseCOM(mVertexLayout);

	delete mSkullObject;
	delete mFloorObject;
	delete mBoxObject;
	delete mSphereObject;
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initiailize Render States
	RenderStates::InitAll(mDevice);
	
	// Initialize Camera
	mCamera.SetPosition(0.0f, 2.0f, -15.0f);

	// Initialize User Input
	InitUserInput();

	// Intialize Lights
	SetupStaticLights();

	// Create Objects
	mFloorObject = new GPlaneXZ(20.0f, 30.0f, 60, 40);
	CreateGeometryBuffers(mFloorObject, false);

	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject, false);

	mSkyObject = new GSky(5000.0f);
	CreateGeometryBuffers(mSkyObject, false);

	// Initialize Object Placement and Properties
	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreateHullShader(&mHullShader, L"Shaders/HullShader.hlsl", "HS");
	CreateDomainShader(&mDomainShader, L"Shaders/DomainShader.hlsl", "DS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");
	CreatePixelShader(&mPixelShaderCube, L"Shaders/PixelShaderCube.hlsl", "PS");

	CreateVertexShader(&mSkyVertexShader, L"Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Shaders/SkyPixelShader.hlsl", "PS");

	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferPSParams, sizeof(ConstBufferPSParams));

	mNormalSetting = false;

	return true;
}

void MyApp::InitUserInput()
{
	mLastMousePos.x = 0;
	mLastMousePos.y = 0;
	bPicked = false;
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
//		Pick(x, y);
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

void MyApp::CreateGeometryBuffers(GObject* obj, bool bDynamic)
{
	D3D11_BUFFER_DESC vbd;
	vbd.ByteWidth = sizeof(Vertex) * obj->GetVertexCount();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.MiscFlags = 0;

	if (bDynamic == true)
	{
		vbd.Usage = D3D11_USAGE_DYNAMIC;
		vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	}
	else 
	{
		vbd.Usage = D3D11_USAGE_IMMUTABLE;
		vbd.CPUAccessFlags = 0;
	}

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = obj->GetVertices();
	HR(mDevice->CreateBuffer(&vbd, &vinitData, obj->GetVertexBuffer()));

	if (obj->IsIndexed())
	{
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
}

void MyApp::PositionObjects()
{
	mFloorObject->SetTextureScaling(8.0f, 10.0f);

	mFloorObject->SetAmbient(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetDiffuse(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mFloorObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	mBoxObject->SetTextureScaling(2.0f, 1.0f);

	mBoxObject->Translate(0.0f, 0.5f, 0.0f);
	mBoxObject->Scale(3.0f, 1.0f, 3.0f);

	mBoxObject->SetAmbient(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mBoxObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	LoadTextureToSRV(mFloorObject->GetDiffuseMapSRV(), L"Textures/floor.dds");
	LoadTextureToSRV(mBoxObject->GetDiffuseMapSRV(), L"Textures/grasscube1024.dds");

	LoadTextureToSRV(mSkyObject->GetDiffuseMapSRV(), L"Textures/grasscube1024.dds");

	LoadTextureToSRV(mFloorObject->GetNormalMapSRV(), L"Textures/floor_nmap.dds");
	LoadTextureToSRV(mBoxObject->GetNormalMapSRV(), L"Textures/bricks_nmap.dds");
}

void MyApp::CreateVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayout));

	VSByteCode->Release();
}

void MyApp::CreateHullShader(ID3D11HullShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* HSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "hs_5_0", D3DCOMPILE_DEBUG, 0, &HSByteCode, 0));

	HR(mDevice->CreateHullShader(HSByteCode->GetBufferPointer(), HSByteCode->GetBufferSize(), NULL, shader));

	HSByteCode->Release();
}

void MyApp::CreateDomainShader(ID3D11DomainShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* DSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "ds_5_0", D3DCOMPILE_DEBUG, 0, &DSByteCode, 0));

	HR(mDevice->CreateDomainShader(DSByteCode->GetBufferPointer(), DSByteCode->GetBufferSize(), NULL, shader));

	DSByteCode->Release();
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

void MyApp::DrawObject(GObject* object)
{
	DirectX::XMMATRIX world = XMLoadFloat4x4(&object->GetWorldTransform());
	Draw(object, world, false);
}

void MyApp::DrawObject(GObject* object, DirectX::XMMATRIX& transform)
{
	DirectX::XMMATRIX world = XMLoadFloat4x4(&object->GetWorldTransform()) * transform;
	Draw(object, world, false);
}

void MyApp::DrawShadow(GObject* object, DirectX::XMMATRIX& transform)
{
	DirectX::XMMATRIX world = XMLoadFloat4x4(&object->GetWorldTransform()) * transform;
	Draw(object, world, true);
}

void MyApp::Draw(GObject* object, DirectX::XMMATRIX& world, bool bShadow)
{
	// Store convenient matrices
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world*mCamera.ViewProj();
	DirectX::XMMATRIX texTransform = XMLoadFloat4x4(&object->GetTexTransform());

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->viewProj = DirectX::XMMatrixTranspose(mCamera.ViewProj());
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(texTransform);

	// If drawing a shadow, use the object's shadow material
	if (bShadow == true) 
	{
		cbPerObject->material = object->GetShadowMaterial(); 
	}
	// Otherwise use the object's normal material
	else { 
		cbPerObject->material = object->GetMaterial(); 
	}

	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	// Set Vertex Buffer to Input Assembler Stage
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, object->GetVertexBuffer(), &stride, &offset);

	// Set Index Buffer to Input Assembler Stage if indexing is enabled for this draw
	if (object->IsIndexed()) 
	{
		mImmediateContext->IASetIndexBuffer(*object->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
	}

	// Add an SRV to the shader for the object's diffuse texture if one exists
	if (object->GetDiffuseMapSRV() != nullptr) 
	{
		mImmediateContext->PSSetShaderResources(0, 1, object->GetDiffuseMapSRV());
	}

	// Add an SRV to the shader for the object's diffuse texture if one exists
	if (object->GetNormalMapSRV() != nullptr)
	{
		mImmediateContext->DSSetShaderResources(0, 1, object->GetNormalMapSRV());
		mImmediateContext->PSSetShaderResources(1, 1, object->GetNormalMapSRV());
	}

	// Draw Object, with indexing if enabled
	if (object->IsIndexed())
	{
		mImmediateContext->DrawIndexed(object->GetIndexCount(), 0, 0);
	}
	else
	{
		mImmediateContext->Draw(object->GetVertexCount(), 0);
	}
}
/*
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
*/
void MyApp::OnResize()
{
	D3DApp::OnResize();

	mCamera.SetLens(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
}

void MyApp::UpdateScene(float dt)
{
	// Control the camera.
	if (GetAsyncKeyState('W') & 0x8000)
	{
		mCamera.Walk(10.0f*dt);
	}

	if (GetAsyncKeyState('S') & 0x8000)
	{
		mCamera.Walk(-10.0f*dt);
	}

	if (GetAsyncKeyState('A') & 0x8000)
	{
		mCamera.Strafe(-10.0f*dt);
	}

	if (GetAsyncKeyState('D') & 0x8000)
	{
		mCamera.Strafe(10.0f*dt);
	}
}

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x31)
	{
		mNormalSetting = true;
	}
	else if (key == 0x32)
	{
		mNormalSetting = false;
	}
}

void MyApp::DrawScene()
{
	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);

	// Update Camera
	mCamera.UpdateViewMatrix();
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->dirLight0 = mDirLights[0];
	cbPerFrame->dirLight1 = mDirLights[1];
	cbPerFrame->dirLight2 = mDirLights[2];
	cbPerFrame->eyePosW = mCamera.GetPosition();
	cbPerFrame->gHeightScale = 0.07f;
	cbPerFrame->gMaxTessDistance = 1.0f;
	cbPerFrame->gMinTessDistance = 25.0f;
	cbPerFrame->gMinTessFactor = 1.0f;
	cbPerFrame->gMaxTessFactor = 5.0f;
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->DSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->DSSetConstantBuffers(1, 1, &mConstBufferPerObject);
	mImmediateContext->DSSetConstantBuffers(2, 1, &mConstBufferPSParams);

	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPSParams);

	mImmediateContext->RSSetState(RenderStates::DefaultRS);
	mImmediateContext->DSSetSamplers(0, 1, &RenderStates::DefaultSS);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->HSSetShader(mHullShader, NULL, 0);
	mImmediateContext->DSSetShader(mDomainShader, NULL, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = mNormalSetting;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	DrawObject(mFloorObject);

	mImmediateContext->PSSetShader(mPixelShaderCube, NULL, 0);

	DrawObject(mBoxObject);
/*
	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = false;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = false;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	DrawObject(mSkullObject);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = true;
	cbPSParams->bUseNormal = false;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	mImmediateContext->PSSetShaderResources(2, 1, mSkyObject->GetDiffuseMapSRV());

	DrawObject(mSphereObject);
*/

	mImmediateContext->HSSetShader(0, NULL, 0);
	mImmediateContext->DSSetShader(0, NULL, 0);

	// Draw Sky
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mImmediateContext->VSSetShader(mSkyVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mSkyPixelShader, NULL, 0);

	mImmediateContext->RSSetState(RenderStates::NoCullRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

	mSkyObject->SetEyePos(mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z);
	DrawObject(mSkyObject);

	HR(mSwapChain->Present(0, 0));
}
