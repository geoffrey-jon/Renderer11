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
	mVertexLayout(0),
	mSkullObject(0),
	mFloorObject(0),
	mBoxObject(0)
{
	mWindowTitle = L"Particle Systems Demo";
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

	ReleaseCOM(mVertexLayout);

	delete mSkullObject;
	delete mFloorObject;
	delete mBoxObject;
	delete [] &mSphereObjects;
	delete [] &mColumnObjects;
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initiailize Render States
	RenderStates::InitAll(mDevice);
	
	// Initialize Camera
	mCamera.SetPosition(0.0f, 6.0f, -10.0f);
	mCamera.Pitch(MathHelper::Pi / 8.0f);

	// Initialize User Input
	InitUserInput();

	// Intialize Lights
	SetupStaticLights();

	// Create Objects
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

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateVertexShader(&mSkyVertexShader, L"Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Shaders/SkyPixelShader.hlsl", "PS");

	CreateVertexShaderParticle(&mParticleStreamOutVS, L"Shaders/ParticleStreamOutVS.hlsl", "VS");
	CreateGeometryShaderStreamOut(&mParticleStreamOutGS, L"Shaders/ParticleStreamOutGS.hlsl", "GS");

	CreateVertexShaderParticle(&mParticleDrawVS, L"Shaders/ParticleDrawVS.hlsl", "VS");
	CreateGeometryShader(&mParticleDrawGS, L"Shaders/ParticleDrawGS.hlsl", "GS");
	CreatePixelShader(&mParticleDrawPS, L"Shaders/ParticleDrawPS.hlsl", "PS");

	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferPSParams, sizeof(ConstBufferPSParams));
	CreateConstantBuffer(&mConstBufferPerFrameParticle, sizeof(ConstBufferPerFrameParticle));

	// Initialize particle system
	mMaxParticles = 500;
	mGameTime = 0.0f;
	mTimeStep = 0.0f;
	mAge = 0.0f;
	mFirstRun = true;
	
	mEmitPosW = DirectX::XMFLOAT3(0.0f, 1.5f, 0.0f);
	mEmitDirW = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);

	BuildParticleVB();
	CreateRandomSRV();

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
		mColumnObjects[i * 2 + 0]->Translate(-8.0f, 1.5f, -12.0f + i*6.0f);
		mColumnObjects[i * 2 + 1]->Translate(+8.0f, 1.5f, -12.0f + i*6.0f);

		mSphereObjects[i * 2 + 0]->Translate(-8.0f, 3.5f, -12.0f + i*6.0f);
		mSphereObjects[i * 2 + 1]->Translate(+8.0f, 3.5f, -12.0f + i*6.0f);
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

	LoadTextureToSRV(&mTexArraySRV, L"Textures/flare0.dds");
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

void MyApp::CreateVertexShaderParticle(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "AGE",      0, DXGI_FORMAT_R32_FLOAT,       0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TYPE",     0, DXGI_FORMAT_R32_UINT,        0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayoutParticle));

	VSByteCode->Release();
}

void MyApp::CreateGeometryShader(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	HR(mDevice->CreateGeometryShader(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), NULL, shader));

	GSByteCode->Release();
}

void MyApp::CreateGeometryShaderStreamOut(ID3D11GeometryShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* GSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "gs_5_0", D3DCOMPILE_DEBUG, 0, &GSByteCode, 0));

	D3D11_SO_DECLARATION_ENTRY soDecl[] =
	{
		// semantic name, semantic index, start component, component count, output slot
		{ 0, "POSITION",    0, 0, 3, 0 },   // output all components of position
		{ 0, "VELOCITY",    0, 0, 3, 0 },     // output the first 3 of the normal
		{ 0, "SIZE",        0, 0, 2, 0 },     // output the first 2 texture coordinates
		{ 0, "AGE",         0, 0, 1, 0 },     // output the first 2 texture coordinates
		{ 0, "TYPE",        0, 0, 1, 0 },     // output the first 2 texture coordinates
	};

	UINT numEntries = sizeof(soDecl) / sizeof(D3D11_SO_DECLARATION_ENTRY);

	HR(mDevice->CreateGeometryShaderWithStreamOutput(GSByteCode->GetBufferPointer(), GSByteCode->GetBufferSize(), soDecl, numEntries, NULL, 0, 0, NULL, shader));

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

	static const DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	
	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->world = DirectX::XMMatrixTranspose(world);
	cbPerObject->worldInvTranpose = DirectX::XMMatrixTranspose(worldInvTranspose);
	cbPerObject->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObject->texTransform = DirectX::XMMatrixTranspose(texTransform);
	cbPerObject->worldViewProjTex = DirectX::XMMatrixTranspose(worldViewProj * T);

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

	mGameTime = mTimer.TotalTime();
	mTimeStep = dt;
	mAge += dt;
}

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x31)
	{
	}
	else if (key == 0x32)
	{
	}
}

void MyApp::CreateRandomSRV()
{
	DirectX::XMFLOAT4 randomValues[1024];
	for (int i = 0; i < 1024; ++i)
	{
		randomValues[i].x = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].y = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].z = MathHelper::RandF(-1.0f, 1.0f);
		randomValues[i].w = MathHelper::RandF(-1.0f, 1.0f);
	}

	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues;
	initData.SysMemPitch = 1024 * sizeof(DirectX::XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	ID3D11Texture1D* tex;
	HR(mDevice->CreateTexture1D(&texDesc, &initData, &tex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	srvDesc.Texture1D.MipLevels = texDesc.MipLevels;
	srvDesc.Texture1D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(tex, &srvDesc, &mRandomSRV));

	ReleaseCOM(tex);
}

void MyApp::BuildParticleVB()
{
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Particle);
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	Particle p;
	p.InitialPos = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	p.InitialVel = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
	p.Size = DirectX::XMFLOAT2(0.0f, 0.0f);
	p.Age = 0.0f;
	p.Type = PT_EMITTER;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &p;

	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mInitVB));

	// Create the ping-pong buffers for stream-out and drawing.
	vbd.ByteWidth = sizeof(Particle) * mMaxParticles;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	HR(mDevice->CreateBuffer(&vbd, 0, &mDrawVB));
	HR(mDevice->CreateBuffer(&vbd, 0, &mStreamOutVB));
}

void MyApp::RenderScene()
{
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

	ID3D11SamplerState* samplers[1] = { RenderStates::DefaultSS};
	mImmediateContext->PSSetSamplers(0, 1, samplers);

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

	// Draw Sky
	mImmediateContext->VSSetShader(mSkyVertexShader, NULL, 0);
	mImmediateContext->PSSetShader(mSkyPixelShader, NULL, 0);

	mImmediateContext->RSSetState(RenderStates::NoCullRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

	mSkyObject->SetEyePos(mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z);
	DrawObject(mSkyObject);

	mImmediateContext->VSSetShader(0, NULL, 0);
	mImmediateContext->PSSetShader(0, NULL, 0);
}

void MyApp::RenderParticleSystem()
{
	mImmediateContext->IASetInputLayout(mVertexLayoutParticle);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	UINT stride = sizeof(Particle);
	UINT offset = 0;

	DirectX::XMFLOAT3 eyePosW = mCamera.GetPosition();

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrameParticle, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameParticleResource);
	cbPerFrameParticle = (ConstBufferPerFrameParticle*)cbPerFrameParticleResource.pData;
	cbPerFrameParticle->eyePosW = DirectX::XMFLOAT4(eyePosW.x, eyePosW.y, eyePosW.z, 0.0f);
	cbPerFrameParticle->emitPosW = DirectX::XMFLOAT4(mEmitPosW.x, mEmitPosW.y, mEmitPosW.z, 0.0f);
	cbPerFrameParticle->emitDirW = DirectX::XMFLOAT4(mEmitDirW.x, mEmitDirW.y, mEmitDirW.z, 0.0f);
	cbPerFrameParticle->gameTime = mGameTime;
	cbPerFrameParticle->timeStep = mTimeStep;
	cbPerFrameParticle->viewProj = DirectX::XMMatrixTranspose(mCamera.ViewProj());
	mImmediateContext->Unmap(mConstBufferPerFrameParticle, 0);

	// Stream-Out Particles

	if (mFirstRun)
	{
		mImmediateContext->IASetVertexBuffers(0, 1, &mInitVB, &stride, &offset);
	}
	else
	{
		mImmediateContext->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
	}

	mImmediateContext->SOSetTargets(1, &mStreamOutVB, &offset);

	mImmediateContext->VSSetShader(mParticleStreamOutVS, NULL, 0);
	mImmediateContext->GSSetShader(mParticleStreamOutGS, NULL, 0);
	mImmediateContext->PSSetShader(0, NULL, 0);

	mImmediateContext->GSSetShaderResources(0, 1, &mRandomSRV);
	mImmediateContext->GSSetConstantBuffers(0, 1, &mConstBufferPerFrameParticle);

	mImmediateContext->GSSetSamplers(0, 1, &RenderStates::DefaultSS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DisableDepthDSS, 0);

	if (mFirstRun)
	{
		mImmediateContext->Draw(1, 0);
		mFirstRun = false;
	}
	else
	{
		mImmediateContext->DrawAuto();
	}

	ID3D11Buffer* nullBuffers[1] = { 0 };
	mImmediateContext->SOSetTargets(1, nullBuffers, &offset);

	// Draw Particles
	std::swap(mDrawVB, mStreamOutVB);

	mImmediateContext->IASetVertexBuffers(0, 1, &mDrawVB, &stride, &offset);
	
	mImmediateContext->VSSetShader(mParticleDrawVS, NULL, 0);
	mImmediateContext->GSSetShader(mParticleDrawGS, NULL, 0);
	mImmediateContext->PSSetShader(mParticleDrawPS, NULL, 0);

	mImmediateContext->GSSetConstantBuffers(0, 1, &mConstBufferPerFrameParticle);

	mImmediateContext->PSSetShaderResources(0, 1, &mTexArraySRV);
	mImmediateContext->PSSetSamplers(0, 1, &RenderStates::DefaultSS);

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->OMSetDepthStencilState(RenderStates::NoDepthWritesDSS, 0);
	mImmediateContext->OMSetBlendState(RenderStates::AdditiveBS, blendFactor, 0xffffffff);

	mImmediateContext->DrawAuto();

	mImmediateContext->VSSetShader(0, NULL, 0);
	mImmediateContext->GSSetShader(0, NULL, 0);
	mImmediateContext->PSSetShader(0, NULL, 0);
}

void MyApp::DrawScene()
{
	// Update Camera
	mCamera.UpdateViewMatrix();

	// Normal Lighting Pass - Render scene to the back buffer
	RenderScene();	

	// Particle System
	RenderParticleSystem();

	HR(mSwapChain->Present(0, 0));
}
