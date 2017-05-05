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
	mPixelShader(0),
	mSkyVertexShader(0),
	mSkyPixelShader(0),
	mShadowVertexShader(0),
	mVertexLayout(0),
	mSkullObject(0),
	mFloorObject(0),
	mBoxObject(0)
{
	mWindowTitle = L"Shadow Map Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();
	RenderStates::DestroyAll();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mConstBufferPSParams);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);

	ReleaseCOM(mSkyVertexShader);
	ReleaseCOM(mSkyPixelShader);

	ReleaseCOM(mShadowVertexShader);

	ReleaseCOM(mVertexLayout);

	delete mSkullObject;
	delete mFloorObject;
	delete mBoxObject;
	delete [] mSphereObjects;
	delete [] mColumnObjects;
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
	mSkullObject = new GObject("Models/skull.txt");
	CreateGeometryBuffers(mSkullObject, false);

	mFloorObject = new GPlaneXZ(20.0f, 30.0f, 60, 40);
	CreateGeometryBuffers(mFloorObject, false);

	mBoxObject = new GCube();
	CreateGeometryBuffers(mBoxObject, false);

	for (int i = 0; i < 10; ++i)
	{
		mSphereObjects[i] = new GSphere();
		CreateGeometryBuffers(mSphereObjects[i], false);
	}

	for (int i = 0; i < 10; ++i)
	{
		mColumnObjects[i] = new GCylinder();
		CreateGeometryBuffers(mColumnObjects[i], false);
	}

	mSkyObject = new GSky(5000.0f);
	CreateGeometryBuffers(mSkyObject, false);

	// Initialize Object Placement and Properties
	PositionObjects();

	// Initialize Shadow Map
	mShadowMap = new ShadowMap(mDevice, 2048, 2048);

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateVertexShader(&mSkyVertexShader, L"Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Shaders/SkyPixelShader.hlsl", "PS");

	CreateVertexShaderShadow(&mShadowVertexShader, L"Shaders/ShadowVertexShader.hlsl", "VS");

	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferPSParams, sizeof(ConstBufferPSParams));
	CreateConstantBuffer(&mConstBufferPerObjectShadow, sizeof(ConstBufferPerObjectShadow));

	mNormalSetting = true;
	mLightRotationAngle = 0.0f;

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
	mFloorObject->SetTextureScaling(4.0f, 4.0f);

	mFloorObject->SetAmbient(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetDiffuse(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f));
	mFloorObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mFloorObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	for (int i = 0; i < 5; ++i)
	{
		mColumnObjects[i * 2 + 0]->Translate(-5.0f, 1.5f, -10.0f + i*5.0f);
		mColumnObjects[i * 2 + 1]->Translate(+5.0f, 1.5f, -10.0f + i*5.0f);

		mSphereObjects[i * 2 + 0]->Translate(-5.0f, 3.5f, -10.0f + i*5.0f);
		mSphereObjects[i * 2 + 1]->Translate(+5.0f, 3.5f, -10.0f + i*5.0f);
	}

	for (int i = 0; i < 10; ++i)
	{
		mSphereObjects[i]->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
		mSphereObjects[i]->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		mSphereObjects[i]->SetSpecular(DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f));
		mSphereObjects[i]->SetReflect(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));

		mColumnObjects[i]->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
		mColumnObjects[i]->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		mColumnObjects[i]->SetSpecular(DirectX::XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f));
		mColumnObjects[i]->SetReflect(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
	}

	mBoxObject->Translate(0.0f, 0.5f, 0.0f);
	mBoxObject->Scale(3.0f, 1.0f, 3.0f);

	mBoxObject->SetAmbient(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetDiffuse(DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	mBoxObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mBoxObject->SetReflect(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));

	mSkullObject->Translate(0.0f, 1.0f, 0.0f);
	mSkullObject->Scale(0.5f, 0.5f, 0.5f);

	mSkullObject->SetAmbient(DirectX::XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f));
	mSkullObject->SetDiffuse(DirectX::XMFLOAT4(0.4f, 0.6f, 0.4f, 1.0f));
	mSkullObject->SetSpecular(DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f));
	mSkullObject->SetReflect(DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f));

	LoadTextureToSRV(mFloorObject->GetDiffuseMapSRV(), L"Textures/floor.dds");
	LoadTextureToSRV(mFloorObject->GetNormalMapSRV(), L"Textures/floor_nmap.dds");

	LoadTextureToSRV(mBoxObject->GetDiffuseMapSRV(), L"Textures/grass.dds");
	LoadTextureToSRV(mBoxObject->GetNormalMapSRV(), L"Textures/bricks_nmap.dds");

	for (int i = 0; i < 10; ++i)
	{
		LoadTextureToSRV(mSphereObjects[i]->GetDiffuseMapSRV(), L"Textures/ice.dds");
		LoadTextureToSRV(mColumnObjects[i]->GetDiffuseMapSRV(), L"Textures/stone.dds");
	}

	LoadTextureToSRV(mSkyObject->GetDiffuseMapSRV(), L"Textures/grasscube1024.dds");
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

void MyApp::CreateVertexShaderShadow(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayoutShadow));

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

	mOriginalLightDir[0] = mDirLights[0].Direction;
	mOriginalLightDir[1] = mDirLights[1].Direction;
	mOriginalLightDir[2] = mDirLights[2].Direction;
}

void MyApp::BuildShadowTransform()
{
	// Build Directional Light View Matrix
	float radius = 18.0f;

	DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mDirLights[0].Direction);
	DirectX::XMVECTOR lightPos = DirectX::XMVectorScale(lightDir, -2.0f*radius);
	DirectX::XMVECTOR targetPos = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX V = DirectX::XMMatrixLookAtLH(lightPos, targetPos, up);

	// Build Directional Light Ortho Projection Volume Matrix
	DirectX::XMFLOAT3 center(0.0f, 0.0f, 0.0f);
	DirectX::XMStoreFloat3(&center, DirectX::XMVector3TransformCoord(targetPos, V));

	DirectX::XMMATRIX P = DirectX::XMMatrixOrthographicOffCenterLH(
		center.x - radius, center.x + radius,
		center.y - radius, center.y + radius,
		center.z - radius, center.z + radius);

	// Build Directional Light Texture Matrix to transform from NDC space to Texture Space
	DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// Multiply View, Projection, and Texture matrices to get Shadow Transformation Matrix
	// Transforms vertices from world space to the view space of the light, to the projection
	// plane, to texture coordinates.
	DirectX::XMMATRIX S = V*P*T;

	DirectX::XMStoreFloat4x4(&mLightView, V);
	DirectX::XMStoreFloat4x4(&mLightProj, P);
	DirectX::XMStoreFloat4x4(&mShadowTransform, S);
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
	DirectX::XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(texTransform);
	cbPerObject->shadowTransform = DirectX::XMMatrixTranspose(world*shadowTransform);

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

	// Animate the lights
//	mLightRotationAngle += 0.25f*dt;

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationY(mLightRotationAngle);
	
	for (int i = 0; i < 3; ++i)
	{
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mOriginalLightDir[i]);
		lightDir = DirectX::XMVector3TransformNormal(lightDir, R);
		DirectX::XMStoreFloat3(&mDirLights[i].Direction, lightDir);
	}

	BuildShadowTransform();
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

void MyApp::RenderShadowMap()
{
	// Set Null Render Target and Shadow Map DSV
	ID3D11RenderTargetView* renderTargets[] = { nullptr };
	ID3D11DepthStencilView* shadowMapDSV = mShadowMap->GetDepthMapDSV();
	mImmediateContext->OMSetRenderTargets(1, renderTargets, shadowMapDSV);

	// Clear Shadow Map DSV
	mImmediateContext->ClearDepthStencilView(shadowMapDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Set Viewport
	D3D11_VIEWPORT shadowMapViewport = mShadowMap->GetViewport();
	mImmediateContext->RSSetViewports(1, &shadowMapViewport);

	// Set Vertex Layout for shadow map
	mImmediateContext->IASetInputLayout(mVertexLayoutShadow);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	mImmediateContext->RSSetState(RenderStates::ShadowMapRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set shadow map VS and null PS
	mImmediateContext->VSSetShader(mShadowVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(NULL, NULL, 0);

	// Set VS constant buffer 
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectShadow);

	// Compute ViewProj matrix of the light source
	DirectX::XMMATRIX view = XMLoadFloat4x4(&mLightView);
	DirectX::XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
	DirectX::XMMATRIX viewProj = XMMatrixMultiply(view, proj);

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldViewProj;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw the grid
	world = DirectX::XMLoadFloat4x4(&mFloorObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mFloorObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mFloorObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mFloorObject->GetIndexCount(), 0, 0);

	// Draw the box
	world = DirectX::XMLoadFloat4x4(&mBoxObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);

	// Draw the cylinders
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mColumnObjects[i]->GetWorldTransform());
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
		cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
		cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mColumnObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mColumnObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mColumnObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the spheres.
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mSphereObjects[i]->GetWorldTransform());
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
		cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
		cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mSphereObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mSphereObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mSphereObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the skull.
	world = DirectX::XMLoadFloat4x4(&mSkullObject->GetWorldTransform());
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectShadow, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectShadowResource);
	cbPerObjectShadow = (ConstBufferPerObjectShadow*)cbPerObjectShadowResource.pData;
	cbPerObjectShadow->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferPerObjectShadow, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mSkullObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mSkullObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mSkullObject->GetIndexCount(), 0, 0);
}

void MyApp::RenderScene()
{
	// Restore the back and dpeth buffer
	ID3D11RenderTargetView* renderTargets[] = { mRenderTargetView };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Set Viewport
	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->RSSetState(RenderStates::DefaultRS);
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set Shaders
	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	ID3D11SamplerState* samplers[2] = { RenderStates::DefaultSS, RenderStates::ShadowMapCompSS };
	mImmediateContext->PSSetSamplers(0, 2, samplers);

	// Update Camera
	mCamera.UpdateViewMatrix();

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->dirLight0 = mDirLights[0];
	cbPerFrame->dirLight1 = mDirLights[1];
	cbPerFrame->dirLight2 = mDirLights[2];
	cbPerFrame->eyePosW = mCamera.GetPosition();
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->VSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);
	mImmediateContext->PSSetConstantBuffers(2, 1, &mConstBufferPSParams);

	ID3D11ShaderResourceView* sceneShadowMap = mShadowMap->GetDepthMapSRV();
	mImmediateContext->PSSetShaderResources(2, 1, &sceneShadowMap);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = true;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	DrawObject(mFloorObject);
	DrawObject(mBoxObject);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = false;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	for (int i = 0; i < 10; ++i)
	{
		DrawObject(mColumnObjects[i]);
	}

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = true;
	cbPSParams->bUseNormal = false;
	mImmediateContext->Unmap(mConstBufferPSParams, 0);

	mImmediateContext->PSSetShaderResources(3, 1, mSkyObject->GetDiffuseMapSRV());

	for (int i = 0; i < 10; ++i)
	{
		DrawObject(mSphereObjects[i]);
	}

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

	// Draw Sky
	mImmediateContext->VSSetShader(mSkyVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mSkyPixelShader, NULL, 0);

	mImmediateContext->RSSetState(RenderStates::NoCullRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

	mSkyObject->SetEyePos(mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z);
	DrawObject(mSkyObject);

	// Unbind Shadow Map SRV
	ID3D11ShaderResourceView* nullSRVs[1] = { NULL };
	mImmediateContext->PSSetShaderResources(2, 1, nullSRVs);
}

void MyApp::DrawScene()
{
	// Shadow Pass - Render scene depth to the shadow map
	RenderShadowMap();
	
	// Normal Lighting Pass - Render scene to the back buffer
	RenderScene();	

	HR(mSwapChain->Present(0, 0));
}
