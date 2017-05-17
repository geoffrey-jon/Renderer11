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
	mConstBufferPerObjectND(0),
	mConstBufferPerFrameSSAO(0),
	mConstBufferBlurParams(0),
	mVertexShader(0),
	mPixelShader(0),
	mSkyVertexShader(0),
	mSkyPixelShader(0),
	mNormalDepthVS(0),
	mNormalDepthPS(0),
	mSsaoVS(0),
	mSsaoPS(0),
	mBlurVS(0),
	mBlurPS(0),
	mVertexLayout(0),
	mVertexLayoutNormalDepth(0),
	mSkullObject(0),
	mFloorObject(0),
	mBoxObject(0)
{
	mWindowTitle = L"Ambient Occlusion Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();
	RenderStates::DestroyAll();

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);
	ReleaseCOM(mConstBufferPSParams);
	ReleaseCOM(mConstBufferPerObjectND);
	ReleaseCOM(mConstBufferPerFrameSSAO);
	ReleaseCOM(mConstBufferBlurParams);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mPixelShader);

	ReleaseCOM(mSkyVertexShader);
	ReleaseCOM(mSkyPixelShader);

	ReleaseCOM(mNormalDepthVS);
	ReleaseCOM(mNormalDepthPS);

	ReleaseCOM(mSsaoVS);
	ReleaseCOM(mSsaoPS);

	ReleaseCOM(mBlurVS);
	ReleaseCOM(mBlurPS);

	ReleaseCOM(mVertexLayout);
	ReleaseCOM(mVertexLayoutNormalDepth);

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

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateVertexShader(&mSkyVertexShader, L"Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Shaders/SkyPixelShader.hlsl", "PS");

	CreateVertexShaderNormalDepth(&mNormalDepthVS, L"Shaders/NormalDepthVS.hlsl", "VS");
	CreatePixelShader(&mNormalDepthPS, L"Shaders/NormalDepthPS.hlsl", "PS");

	CreateVertexShaderSSAO(&mSsaoVS, L"Shaders/SSAOVS.hlsl", "VS");
	CreatePixelShader(&mSsaoPS, L"Shaders/SSAOPS.hlsl", "PS");

	CreateVertexShaderSSAO(&mBlurVS, L"Shaders/BlurVS.hlsl", "VS");
	CreatePixelShader(&mBlurPS, L"Shaders/BlurPS.hlsl", "PS");

	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferPSParams, sizeof(ConstBufferPSParams));
	CreateConstantBuffer(&mConstBufferPerObjectND, sizeof(ConstBufferPerObjectNormalDepth));
	CreateConstantBuffer(&mConstBufferPerFrameSSAO, sizeof(ConstBufferPerFrameSSAO));
	CreateConstantBuffer(&mConstBufferBlurParams, sizeof(ConstBufferBlurParams));

	SetupNormalDepth();
	SetupSSAO();
	BuildFrustumCorners();
	BuildOffsetVectors();
	BuildFullScreenQuad();
	BuildRandomVectorTexture();
	mAOSetting = false;

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

void MyApp::CreateVertexShaderNormalDepth(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayoutNormalDepth));

	VSByteCode->Release();
}

void MyApp::CreateVertexShaderSSAO(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayoutSSAO));

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
}

void MyApp::OnKeyDown(WPARAM key, LPARAM info)
{
	if (key == 0x31)
	{
		mAOSetting = true;
	}
	else if (key == 0x32)
	{
		mAOSetting = false;
	}
}

void MyApp::SetupNormalDepth()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC normalDepthTexDesc;
	normalDepthTexDesc.Width = mClientWidth;
	normalDepthTexDesc.Height = mClientHeight;
	normalDepthTexDesc.MipLevels = 1;
	normalDepthTexDesc.ArraySize = 1;
	normalDepthTexDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthTexDesc.SampleDesc.Count = 1;
	normalDepthTexDesc.SampleDesc.Quality = 0;
	normalDepthTexDesc.Usage = D3D11_USAGE_DEFAULT;
	normalDepthTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	normalDepthTexDesc.CPUAccessFlags = 0;
	normalDepthTexDesc.MiscFlags = 0;

	ID3D11Texture2D* normalDepthTex;
	HR(mDevice->CreateTexture2D(&normalDepthTexDesc, 0, &normalDepthTex));

	D3D11_RENDER_TARGET_VIEW_DESC normalDepthRTVDesc;
	normalDepthRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	normalDepthRTVDesc.Texture2D.MipSlice = 0;

	HR(mDevice->CreateRenderTargetView(normalDepthTex, &normalDepthRTVDesc, &mNormalDepthRTV));

	D3D11_SHADER_RESOURCE_VIEW_DESC normalDepthSRVDesc;
	normalDepthSRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	normalDepthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	normalDepthSRVDesc.Texture2D.MipLevels = normalDepthTexDesc.MipLevels;
	normalDepthSRVDesc.Texture2D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(normalDepthTex, &normalDepthSRVDesc, &mNormalDepthSRV));

	ReleaseCOM(normalDepthTex);
}

void MyApp::SetupSSAO()
{
	// Are the texture formats correct?

	D3D11_TEXTURE2D_DESC ssaoTexDesc;
	ssaoTexDesc.Width = mClientWidth;
	ssaoTexDesc.Height = mClientHeight;
	ssaoTexDesc.MipLevels = 1;
	ssaoTexDesc.ArraySize = 1;
	ssaoTexDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoTexDesc.SampleDesc.Count = 1;
	ssaoTexDesc.SampleDesc.Quality = 0;
	ssaoTexDesc.Usage = D3D11_USAGE_DEFAULT;
	ssaoTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ssaoTexDesc.CPUAccessFlags = 0;
	ssaoTexDesc.MiscFlags = 0;

	ID3D11Texture2D* ssaoTex0;
	HR(mDevice->CreateTexture2D(&ssaoTexDesc, 0, &ssaoTex0));

	ID3D11Texture2D* ssaoTex1;
	HR(mDevice->CreateTexture2D(&ssaoTexDesc, 0, &ssaoTex1));

	D3D11_RENDER_TARGET_VIEW_DESC ssaoRTVDesc;
	ssaoRTVDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ssaoRTVDesc.Texture2D.MipSlice = 0;

	HR(mDevice->CreateRenderTargetView(ssaoTex0, &ssaoRTVDesc, &mSsaoRTV0));
	HR(mDevice->CreateRenderTargetView(ssaoTex1, &ssaoRTVDesc, &mSsaoRTV1));

	D3D11_SHADER_RESOURCE_VIEW_DESC ssaoSRVDesc;
	ssaoSRVDesc.Format = DXGI_FORMAT_R16_FLOAT;
	ssaoSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ssaoSRVDesc.Texture2D.MipLevels = ssaoTexDesc.MipLevels;
	ssaoSRVDesc.Texture2D.MostDetailedMip = 0;

	HR(mDevice->CreateShaderResourceView(ssaoTex0, &ssaoSRVDesc, &mSsaoSRV0));
	HR(mDevice->CreateShaderResourceView(ssaoTex1, &ssaoSRVDesc, &mSsaoSRV1));

	ReleaseCOM(ssaoTex0);
	ReleaseCOM(ssaoTex1);
}

void MyApp::BuildFrustumCorners()
{
	float farZ = mCamera.GetFarZ();
	float aspect = mCamera.GetAspect();
	float halfHeight = farZ * tanf(0.5f * mCamera.GetFovY());
	float halfWidth = aspect * halfHeight;

	mFrustumFarCorners[0] = DirectX::XMFLOAT4(-halfWidth, -halfHeight, farZ, 0.0f);
	mFrustumFarCorners[1] = DirectX::XMFLOAT4(-halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[2] = DirectX::XMFLOAT4(+halfWidth, +halfHeight, farZ, 0.0f);
	mFrustumFarCorners[3] = DirectX::XMFLOAT4(+halfWidth, -halfHeight, farZ, 0.0f);
}

void MyApp::BuildOffsetVectors()
{
	mOffsets[0] = DirectX::XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[1] = DirectX::XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[2] = DirectX::XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	mOffsets[3] = DirectX::XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	mOffsets[4] = DirectX::XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);
	mOffsets[5] = DirectX::XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);

	mOffsets[6] = DirectX::XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	mOffsets[7] = DirectX::XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	mOffsets[8] = DirectX::XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	mOffsets[9] = DirectX::XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	mOffsets[10] = DirectX::XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);
	mOffsets[11] = DirectX::XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);

	mOffsets[12] = DirectX::XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);
	mOffsets[13] = DirectX::XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);

	// TODO: Add Random Offsets
	for (int i = 0; i < 14; ++i)
	{
		DirectX::XMVECTOR v = DirectX::XMVector2Normalize(DirectX::XMLoadFloat4(&mOffsets[i]));
		float s = MathHelper::RandF(0.25, 1.0f);
		v = DirectX::XMVectorScale(v, s);
		DirectX::XMStoreFloat4(&mOffsets[i], v);
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

void MyApp::BuildRandomVectorTexture()
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 256;
	texDesc.Height = 256;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA initData = { 0 };
	initData.SysMemPitch = 256 * sizeof(DirectX::PackedVector::XMCOLOR);

	DirectX::PackedVector::XMCOLOR color[256 * 256];
	for (int i = 0; i < 256; ++i)
	{
		for (int j = 0; j < 256; ++j)
		{
			DirectX::XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

			color[i * 256 + j] = DirectX::PackedVector::XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	initData.pSysMem = color;

	ID3D11Texture2D* randomVectorTex = 0;
	HR(mDevice->CreateTexture2D(&texDesc, &initData, &randomVectorTex));

	HR(mDevice->CreateShaderResourceView(randomVectorTex, 0, &mRandomVectorSRV));

	ReleaseCOM(randomVectorTex);
}

void MyApp::RenderNormalDepthMap()
{
	ID3D11RenderTargetView* renderTargets[] = { mNormalDepthRTV };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, mDepthStencilView);
	
	// Clear the render target and depth/stencil views
	float clearColor[] = { 0.0f, 0.0f, -1.0f, 1e5f };
	mImmediateContext->ClearRenderTargetView(mNormalDepthRTV, clearColor);
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


	// Set Viewport
	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayoutNormalDepth);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Render States
	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->RSSetState(RenderStates::DefaultRS);
	mImmediateContext->OMSetBlendState(RenderStates::DefaultBS, blendFactor, 0xffffffff);
	mImmediateContext->OMSetDepthStencilState(RenderStates::DefaultDSS, 0);

	// Set Shaders
	mImmediateContext->VSSetShader(mNormalDepthVS, NULL, 0);
	mImmediateContext->PSSetShader(mNormalDepthPS, NULL, 0);

	// Update Camera
	mCamera.UpdateViewMatrix();

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerObjectND);

	// Compute ViewProj matrix of the light source
	DirectX::XMMATRIX view = mCamera.View();
	DirectX::XMMATRIX viewProj = mCamera.ViewProj();

	DirectX::XMMATRIX world;
	DirectX::XMMATRIX worldView;
	DirectX::XMMATRIX worldViewProj;

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Draw the grid
	world = DirectX::XMLoadFloat4x4(&mFloorObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mFloorObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mFloorObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mFloorObject->GetIndexCount(), 0, 0);

	// Draw the box
	world = DirectX::XMLoadFloat4x4(&mBoxObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mBoxObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mBoxObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mBoxObject->GetIndexCount(), 0, 0);

	// Draw the cylinders
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mColumnObjects[i]->GetWorldTransform());
		worldView = world*view;
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
		cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
		cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
		cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
		mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mColumnObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mColumnObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mColumnObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the spheres.
	for (int i = 0; i < 10; ++i)
	{
		world = DirectX::XMLoadFloat4x4(&mSphereObjects[i]->GetWorldTransform());
		worldView = world*view;
		worldViewProj = world*viewProj;

		mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
		cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
		cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
		cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
		cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
		mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

		mImmediateContext->IASetVertexBuffers(0, 1, mSphereObjects[i]->GetVertexBuffer(), &stride, &offset);
		mImmediateContext->IASetIndexBuffer(*mSphereObjects[i]->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

		mImmediateContext->DrawIndexed(mSphereObjects[i]->GetIndexCount(), 0, 0);
	}

	// Draw the skull.
	world = DirectX::XMLoadFloat4x4(&mSkullObject->GetWorldTransform());
	worldView = world*view;
	worldViewProj = world*viewProj;

	mImmediateContext->Map(mConstBufferPerObjectND, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectNDResource);
	cbPerObjectND = (ConstBufferPerObjectNormalDepth*)cbPerObjectNDResource.pData;
	cbPerObjectND->worldView = DirectX::XMMatrixTranspose(worldView);
	cbPerObjectND->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	cbPerObjectND->worldInvTranposeView = MathHelper::InverseTranspose(world)*view;
	mImmediateContext->Unmap(mConstBufferPerObjectND, 0);

	mImmediateContext->IASetVertexBuffers(0, 1, mSkullObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mSkullObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->DrawIndexed(mSkullObject->GetIndexCount(), 0, 0);
}

void MyApp::RenderSSAOMap()
{
	// TODO: Use half-resolution render target and viewport

	// Restore the back and depth buffer
	ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV0 };
	mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

	// Clear the render target and depth/stencil views
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

	// Set Viewport
	mImmediateContext->RSSetViewports(1, &mViewport);

	// Set Vertex Layout
	mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	static const DirectX::XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	DirectX::XMMATRIX viewTexTransform = XMMatrixMultiply(mCamera.Proj(), T);

	mImmediateContext->Map(mConstBufferPerFrameSSAO, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameSSAOResource);
	cbPerFrameSSAO = (ConstBufferPerFrameSSAO*)cbPerFrameSSAOResource.pData;
	cbPerFrameSSAO->viewTex = DirectX::XMMatrixTranspose(viewTexTransform);
	cbPerFrameSSAO->frustumFarCorners[0] = mFrustumFarCorners[0];
	cbPerFrameSSAO->frustumFarCorners[1] = mFrustumFarCorners[1];
	cbPerFrameSSAO->frustumFarCorners[2] = mFrustumFarCorners[2];
	cbPerFrameSSAO->frustumFarCorners[3] = mFrustumFarCorners[3];
	cbPerFrameSSAO->offsets[0] = mOffsets[0];
	cbPerFrameSSAO->offsets[1] = mOffsets[1];
	cbPerFrameSSAO->offsets[2] = mOffsets[2];
	cbPerFrameSSAO->offsets[3] = mOffsets[3];
	cbPerFrameSSAO->offsets[4] = mOffsets[4];
	cbPerFrameSSAO->offsets[5] = mOffsets[5];
	cbPerFrameSSAO->offsets[6] = mOffsets[6];
	cbPerFrameSSAO->offsets[7] = mOffsets[7];
	cbPerFrameSSAO->offsets[8] = mOffsets[8];
	cbPerFrameSSAO->offsets[9] = mOffsets[9];
	cbPerFrameSSAO->offsets[10] = mOffsets[10];
	cbPerFrameSSAO->offsets[11] = mOffsets[11];
	cbPerFrameSSAO->offsets[12] = mOffsets[12];
	cbPerFrameSSAO->offsets[13] = mOffsets[13];
	mImmediateContext->Unmap(mConstBufferPerFrameSSAO, 0);

	mImmediateContext->VSSetShader(mSsaoVS, NULL, 0);
	mImmediateContext->PSSetShader(mSsaoPS, NULL, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferPerFrameSSAO);
	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrameSSAO);

	mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
	mImmediateContext->PSSetShaderResources(1, 1, &mRandomVectorSRV);

	ID3D11SamplerState* samplers[] = { RenderStates::SsaoSS, RenderStates::DefaultSS };
	mImmediateContext->PSSetSamplers(0, 2, samplers);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	
	mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
	mImmediateContext->DrawIndexed(6, 0, 0);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	mImmediateContext->PSSetShaderResources(0, 1, nullSRVs);
}

void MyApp::BlurSSAOMap(int numBlurs)
{
	for (int i = 0; i < numBlurs; ++i)
	{
		// Horizontal Blur
		ID3D11RenderTargetView* renderTargets[] = { mSsaoRTV1 };
		mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

		// Clear the render target and depth/stencil views
		mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

		// Set Viewport
		mImmediateContext->RSSetViewports(1, &mViewport);

		mImmediateContext->Map(mConstBufferBlurParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbBlurParamsResource);
		cbBlurParams = (ConstBufferBlurParams*)cbBlurParamsResource.pData;
		cbBlurParams->texelWidth = 1.0f / mViewport.Width;
		cbBlurParams->texelHeight = 0.0f;
		mImmediateContext->Unmap(mConstBufferBlurParams, 0);

		// Set Vertex Layout
		mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mImmediateContext->VSSetShader(mBlurVS, NULL, 0);
		mImmediateContext->PSSetShader(mBlurPS, NULL, 0);

		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferBlurParams);

		mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		mImmediateContext->PSSetShaderResources(1, 1, &mSsaoSRV0);

		ID3D11SamplerState* samplers[] = { RenderStates::BlurSS };
		mImmediateContext->PSSetSamplers(0, 1, samplers);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;

		mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mImmediateContext->DrawIndexed(6, 0, 0);

		ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
		mImmediateContext->PSSetShaderResources(0, 2, nullSRVs);



		// Vertical Blur
		renderTargets[0] = mSsaoRTV0;
		mImmediateContext->OMSetRenderTargets(1, renderTargets, 0);

		// Clear the render target and depth/stencil views
		mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Black));

		// Set Viewport
		mImmediateContext->RSSetViewports(1, &mViewport);

		mImmediateContext->Map(mConstBufferBlurParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbBlurParamsResource);
		cbBlurParams = (ConstBufferBlurParams*)cbBlurParamsResource.pData;
		cbBlurParams->texelWidth = 0.0f;
		cbBlurParams->texelHeight = 1.0f / mViewport.Height;
		mImmediateContext->Unmap(mConstBufferBlurParams, 0);

		// Set Vertex Layout
		mImmediateContext->IASetInputLayout(mVertexLayoutSSAO);
		mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		mImmediateContext->VSSetShader(mBlurVS, NULL, 0);
		mImmediateContext->PSSetShader(mBlurPS, NULL, 0);

		mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferBlurParams);

		mImmediateContext->PSSetShaderResources(0, 1, &mNormalDepthSRV);
		mImmediateContext->PSSetShaderResources(1, 1, &mSsaoSRV1);

		mImmediateContext->PSSetSamplers(0, 1, samplers);

		mImmediateContext->IASetVertexBuffers(0, 1, &mScreenQuadVB, &stride, &offset);
		mImmediateContext->IASetIndexBuffer(mScreenQuadIB, DXGI_FORMAT_R16_UINT, 0);
		mImmediateContext->DrawIndexed(6, 0, 0);

		mImmediateContext->PSSetShaderResources(0, 2, nullSRVs);
	}
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

	mImmediateContext->PSSetShaderResources(2, 1, &mSsaoSRV1);

	// Set PS Parameters
	mImmediateContext->Map(mConstBufferPSParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPSParamsResource);
	cbPSParams = (ConstBufferPSParams*)cbPSParamsResource.pData;
	cbPSParams->bUseTexure = true;
	cbPSParams->bAlphaClip = false;
	cbPSParams->bFogEnabled = false;
	cbPSParams->bReflection = false;
	cbPSParams->bUseNormal = true;
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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
	cbPSParams->bUseAO = mAOSetting;
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
	// Render Scene Normals and Depth
	RenderNormalDepthMap();

	// Render SSAO Map
	RenderSSAOMap();

	// Blur SSAO Map
	BlurSSAOMap(4);

	// Normal Lighting Pass - Render scene to the back buffer
	RenderScene();	

	HR(mSwapChain->Present(0, 0));
}
