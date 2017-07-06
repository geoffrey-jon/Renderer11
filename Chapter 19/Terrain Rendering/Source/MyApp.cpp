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
	mVertexShader(0),
	mHullShader(0),
	mDomainShader(0),
	mPixelShader(0),
	mVertexLayout(0),
	mTerrainVB(0),
	mTerrainIB(0)
{
	mWindowTitle = L"Tess Hills Demo";
}

MyApp::~MyApp()
{
	mImmediateContext->ClearState();
	RenderStates::DestroyAll();

	ReleaseCOM(mTerrainVB);
	ReleaseCOM(mTerrainIB);

	ReleaseCOM(mConstBufferPerFrame);
	ReleaseCOM(mConstBufferPerObject);

	ReleaseCOM(mVertexShader);
	ReleaseCOM(mHullShader);
	ReleaseCOM(mDomainShader);
	ReleaseCOM(mPixelShader);

	ReleaseCOM(mVertexLayout);
}

bool MyApp::Init()
{
	// Initialize parent D3DApp
	if (!D3DApp::Init()) { return false; }

	// Initiailize Render States
	RenderStates::InitAll(mDevice);
	
	// Initialize Camera
	mCamera.SetPosition(30.0f, 30.0f, 0.0f);
	mCamera.RotateY(30.0f);
	mCamera.Pitch(340.0f);

	// Initialize User Input
	InitUserInput();

	// Create Sky
	mSkyObject = new GSky(5000.0f);
	CreateGeometryBuffers(mSkyObject, false);

	PositionObjects();

	// Compile Shaders
	CreateVertexShader(&mVertexShader, L"Shaders/VertexShader.hlsl", "VS");
	CreateHullShader(&mHullShader, L"Shaders/HullShader.hlsl", "HS");
	CreateDomainShader(&mDomainShader, L"Shaders/DomainShader.hlsl", "DS");
	CreatePixelShader(&mPixelShader, L"Shaders/PixelShader.hlsl", "PS");

	CreateSkyVertexShader(&mSkyVertexShader, L"Shaders/SkyVertexShader.hlsl", "VS");
	CreatePixelShader(&mSkyPixelShader, L"Shaders/SkyPixelShader.hlsl", "PS");

	// Create Constant Buffers
	CreateConstantBuffer(&mConstBufferPerFrame, sizeof(ConstBufferPerFrame));
	CreateConstantBuffer(&mConstBufferPerObject, sizeof(ConstBufferPerObject));
	CreateConstantBuffer(&mConstBufferWVP, sizeof(ConstBufferWVP));

	mNumCellsWide = 2048;
	mNumCellsDeep = 2048;
	mCellWidth = 0.5; // meters
	mCellDepth = 0.5; // meters

	mNumPatchVertRows = 33;
	mNumPatchVertCols = 33;

	mNumPatches = 32 * 32;

	mTerrainWidth = mNumCellsWide * mCellWidth; // 1024 meters
	mTerrainDepth = mNumCellsDeep * mCellDepth; // 1024 meters

	LoadHeightmap(L"Textures/terrain.raw");
	LoadTextureToSRV(&mBlendMapSRV, L"Textures/blend.dds");
	BuildHeightmapSRV();
	BuildLayerMapSRV();
	BuildTerrainBuffers();
	SetupStaticLights();

	mTerrainMaterial.Ambient = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTerrainMaterial.Diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mTerrainMaterial.Specular = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 64.0f);
	mTerrainMaterial.Reflect = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

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
	LoadTextureToSRV(mSkyObject->GetDiffuseMapSRV(), L"Textures/snowcube1024.dds");
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
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mVertexLayout));

	VSByteCode->Release();
}

void MyApp::CreateSkyVertexShader(ID3D11VertexShader** shader, LPCWSTR filename, LPCSTR entryPoint)
{
	ID3DBlob* VSByteCode = 0;
	HR(D3DCompileFromFile(filename, 0, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, "vs_5_0", D3DCOMPILE_DEBUG, 0, &VSByteCode, 0));

	HR(mDevice->CreateVertexShader(VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), NULL, shader));

	// Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements = sizeof(vertexDesc) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Create the input layout
	HR(mDevice->CreateInputLayout(vertexDesc, numElements, VSByteCode->GetBufferPointer(), VSByteCode->GetBufferSize(), &mSkyVertexLayout));

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

void MyApp::DrawScene()
{
	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::Silver));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Update Camera
	mCamera.UpdateViewMatrix();

	float blendFactor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	mImmediateContext->IASetInputLayout(mVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	// Set per frame constants
	mImmediateContext->Map(mConstBufferPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerFrameResource);
	cbPerFrame = (ConstBufferPerFrame*)cbPerFrameResource.pData;
	cbPerFrame->lights[0] = mDirLights[0];
	cbPerFrame->lights[1] = mDirLights[1];
	cbPerFrame->lights[2] = mDirLights[2];
	cbPerFrame->eyePosW = mCamera.GetPosition();
	cbPerFrame->minDist = 20.0f;
	cbPerFrame->maxDist = 500.0f;
	cbPerFrame->minTess = 0.0f;
	cbPerFrame->maxTess = 6.0f;
	cbPerFrame->texelCellSpaceU = 1.0f / 2049;
	cbPerFrame->texelCellSpaceV = 1.0f / 2049;
	cbPerFrame->worldCellSpace = 0.5f;
	mImmediateContext->Unmap(mConstBufferPerFrame, 0);

	// Bind Constant Buffers to the Pipeline
	mImmediateContext->HSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->HSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->DSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->PSSetConstantBuffers(0, 1, &mConstBufferPerFrame);
	mImmediateContext->PSSetConstantBuffers(1, 1, &mConstBufferPerObject);

	mImmediateContext->RSSetState(RenderStates::DefaultRS);

	mImmediateContext->VSSetShader(mVertexShader, NULL, 0);
	mImmediateContext->HSSetShader(mHullShader, NULL, 0);
	mImmediateContext->DSSetShader(mDomainShader, NULL, 0);
	mImmediateContext->PSSetShader(mPixelShader, NULL, 0);

	// Store convenient matrices
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	DirectX::XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
	DirectX::XMMATRIX worldViewProj = world*mCamera.ViewProj();

	// Set per object constants
	mImmediateContext->Map(mConstBufferPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbPerObjectResource);
	cbPerObject = (ConstBufferPerObject*)cbPerObjectResource.pData;
	cbPerObject->viewProj = DirectX::XMMatrixTranspose(mCamera.ViewProj());
	cbPerObject->material = mTerrainMaterial;
	mImmediateContext->Unmap(mConstBufferPerObject, 0);

	mImmediateContext->VSSetShaderResources(0, 1, &mHeightMapSRV);
	mImmediateContext->DSSetShaderResources(0, 1, &mHeightMapSRV);

	mImmediateContext->PSSetShaderResources(0, 1, &mHeightMapSRV);
	mImmediateContext->PSSetShaderResources(1, 1, &mLayerMapSRV);
	mImmediateContext->PSSetShaderResources(2, 1, &mBlendMapSRV);

	ID3D11SamplerState* samplers[2] = { RenderStates::PointClampSS, RenderStates::DefaultSS };
	mImmediateContext->VSSetSamplers(0, 2, samplers);
	mImmediateContext->DSSetSamplers(0, 2, samplers);
	mImmediateContext->PSSetSamplers(0, 2, samplers);

	// Set Vertex Buffer to Input Assembler Stage
	UINT stride = sizeof(TerrainVertex);
	UINT offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, &mTerrainVB, &stride, &offset);
	mImmediateContext->IASetIndexBuffer(mTerrainIB, DXGI_FORMAT_R16_UINT, 0);

	mImmediateContext->DrawIndexed(mNumPatches * 4, 0, 0);

	// Draw Sky
	mImmediateContext->IASetInputLayout(mSkyVertexLayout);
	mImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	mImmediateContext->VSSetShader(mSkyVertexShader, NULL, 0);
	mImmediateContext->HSSetShader(0, NULL, 0);
	mImmediateContext->DSSetShader(0, NULL, 0);
	mImmediateContext->PSSetShader(mSkyPixelShader, NULL, 0);

	mImmediateContext->RSSetState(RenderStates::NoCullRS);
	mImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS, 0);

	mSkyObject->SetEyePos(mCamera.GetPosition().x, mCamera.GetPosition().y, mCamera.GetPosition().z);

	// Store convenient matrices
	world = DirectX::XMLoadFloat4x4(&mSkyObject->GetWorldTransform());
	worldViewProj = world*mCamera.ViewProj();

	// Set per object constants
	mImmediateContext->Map(mConstBufferWVP, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbWVPResource);
	cbWVP = (ConstBufferWVP*)cbWVPResource.pData;
	cbWVP->worldViewProj = DirectX::XMMatrixTranspose(worldViewProj);
	mImmediateContext->Unmap(mConstBufferWVP, 0);

	mImmediateContext->VSSetConstantBuffers(0, 1, &mConstBufferWVP);

	// Set Vertex Buffer to Input Assembler Stage
	stride = sizeof(Vertex);
	offset = 0;

	mImmediateContext->IASetVertexBuffers(0, 1, mSkyObject->GetVertexBuffer(), &stride, &offset);
	mImmediateContext->IASetIndexBuffer(*mSkyObject->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);

	mImmediateContext->PSSetShaderResources(0, 1, mSkyObject->GetDiffuseMapSRV());

	mImmediateContext->DrawIndexed(mSkyObject->GetIndexCount(), 0, 0);

	HR(mSwapChain->Present(0, 0));
}

void MyApp::BuildTerrainBuffers()
{
	/* 
		Max Terrain Size - 2048 cells x 2048 cells
						 - 2049 verts x 2049 verts
		Max Patch Size - 64 cells x 64 cells
		Terrain Size in Patches - 32 patches x 32 patches
								- 32 quads x 32 quads
		Required verts for quads = 33 x 33
	*/

	std::vector<TerrainVertex> verts(mNumPatchVertRows*mNumPatchVertCols);

	float halfWidth = 0.5f*mTerrainWidth; // 512 meters
	float halfDepth = 0.5f*mTerrainDepth; // 512 meters

	float patchWidth = mTerrainWidth / (mNumPatchVertCols - 1); // 32 meters
	float patchDepth = mTerrainDepth / (mNumPatchVertRows - 1); // 32 meters
	float du = 1.0f / (mNumPatchVertCols - 1); // 1/32
	float dv = 1.0f / (mNumPatchVertRows - 1); // 1/32

	for (UINT i = 0; i < mNumPatchVertRows; ++i)
	{
		float z = halfDepth - i*patchDepth; // z = [512, 480, ..., 0, ..., -480, -512]
		for (UINT j = 0; j < mNumPatchVertCols; ++j)
		{
			float x = -halfWidth + j*patchWidth; // x = [512, 480, ..., 0, ..., -480, -512]

			verts[i*mNumPatchVertCols + j].Pos = DirectX::XMFLOAT3(x, 0.0f, z);

			// Stretch texture over grid.
			verts[i*mNumPatchVertCols + j].Tex.x = j*du; // u = [0, 1/32, ..., 31/32, 1]
			verts[i*mNumPatchVertCols + j].Tex.y = i*dv; // v = [0, 1/32, ..., 31/32, 1]

			verts[i*mNumPatchVertCols + j].BoundsY = DirectX::XMFLOAT2(0.0f, 0.0f);
		}
	}

	// Store axis-aligned bounding box y-bounds in upper-left patch corner.
//	for (UINT i = 0; i < mNumPatchVertRows - 1; ++i)
//	{
//		for (UINT j = 0; j < mNumPatchVertCols - 1; ++j)
//		{
//			UINT patchID = i*(mNumPatchVertCols - 1) + j;
//			verts[i*mNumPatchVertCols + j].BoundsY = mPatchBoundsY[patchID];
//		}
//	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(TerrainVertex) * verts.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &verts[0];
	HR(mDevice->CreateBuffer(&vbd, &vinitData, &mTerrainVB));



	std::vector<USHORT> indices(mNumPatches * 4); // 4 indices per quad face

	// Iterate over each quad and compute indices.
	int k = 0;
	for (UINT i = 0; i < mNumPatchVertRows - 1; ++i)
	{
		for (UINT j = 0; j < mNumPatchVertCols - 1; ++j)
		{
			// Top row of 2x2 quad patch
			indices[k] = i*mNumPatchVertCols + j;
			indices[k + 1] = i*mNumPatchVertCols + j + 1;

			// Bottom row of 2x2 quad patch
			indices[k + 2] = (i + 1)*mNumPatchVertCols + j;
			indices[k + 3] = (i + 1)*mNumPatchVertCols + j + 1;

			k += 4; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(USHORT) * indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
	HR(mDevice->CreateBuffer(&ibd, &iinitData, &mTerrainIB));
}

void MyApp::LoadHeightmap(LPCWSTR filename)
{
	// A height for each vertex
	std::vector<unsigned char> in(2049 * 2049);

	// Open the file.
	std::ifstream inFile;
	inFile.open(filename, std::ios_base::binary);

	if (inFile)
	{
		// Read the RAW bytes.
		inFile.read((char*)&in[0], (std::streamsize)in.size());

		// Done with file.
		inFile.close();
	}

	// Copy the array data into a float array and scale it.
	mHeightmap.resize(2049 * 2049, 0);
	float mHeightScale = 50.0f;
	for (UINT i = 0; i < 2049 * 2049; ++i)
	{
		mHeightmap[i] = (in[i] / 255.0f)*mHeightScale;
	}
}

void MyApp::BuildHeightmapSRV()
{
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = 2049;
	texDesc.Height = 2049;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = &mHeightmap[0];
	data.SysMemPitch = 2049*sizeof(float);
	data.SysMemSlicePitch = 0;

	ID3D11Texture2D* hmapTex = 0;
	HR(mDevice->CreateTexture2D(&texDesc, &data, &hmapTex));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	HR(mDevice->CreateShaderResourceView(hmapTex, &srvDesc, &mHeightMapSRV));

	// SRV saves reference.
	ReleaseCOM(hmapTex);
}

void MyApp::BuildLayerMapSRV()
{
	std::vector<ID3D11Resource*> texResources;
	texResources.resize(5);

	HR(DirectX::CreateDDSTextureFromFileEx(mDevice, L"Textures/grass.dds", 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, &texResources[0], nullptr));
	HR(DirectX::CreateDDSTextureFromFileEx(mDevice, L"Textures/darkdirt.dds", 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, &texResources[1], nullptr));
	HR(DirectX::CreateDDSTextureFromFileEx(mDevice, L"Textures/stone.dds", 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, &texResources[2], nullptr));
	HR(DirectX::CreateDDSTextureFromFileEx(mDevice, L"Textures/lightdirt.dds", 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, &texResources[3], nullptr));
	HR(DirectX::CreateDDSTextureFromFileEx(mDevice, L"Textures/snow.dds", 0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ, 0, false, &texResources[4], nullptr));

	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width = 512;
	texArrayDesc.Height = 512;
	texArrayDesc.MipLevels = 10;
	texArrayDesc.ArraySize = 5;
	texArrayDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	texArrayDesc.SampleDesc.Count = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;
	texArrayDesc.MiscFlags = 0;

	HR(mDevice->CreateTexture2D(&texArrayDesc, 0, &mTexArray));

	// Copy individual texture elements into texture array.
	for (UINT texElement = 0; texElement < 5; ++texElement)
	{
		for (UINT mipLevel = 0; mipLevel < 10; ++mipLevel)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			HR(mImmediateContext->Map(texResources[texElement], mipLevel, D3D11_MAP_READ, 0, &mappedTex2D));

			mImmediateContext->UpdateSubresource(mTexArray,
				D3D11CalcSubresource(mipLevel, texElement, 10),
				0, mappedTex2D.pData, mappedTex2D.RowPitch, mappedTex2D.DepthPitch);

			mImmediateContext->Unmap(texResources[texElement], mipLevel);
		}
	}

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = 5;

	HR(mDevice->CreateShaderResourceView(mTexArray, &viewDesc, &mLayerMapSRV));

	// Cleanup--we only need the resource view.
	ReleaseCOM(mTexArray);

	for (UINT i = 0; i < 5; ++i)
	{
		ReleaseCOM(texResources[i]);
	}
}