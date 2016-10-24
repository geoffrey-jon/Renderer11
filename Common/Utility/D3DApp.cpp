/*  =============================================
	Summary: Direct3D Application Framework Class
	=============================================  */

#include "D3DApp.h"

namespace
{
	D3DApp* gD3DApp = 0;
}

LRESULT CALLBACK MainWindProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return gD3DApp->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp::D3DApp(HINSTANCE Instance) :
	mAppInstance(Instance),
	mClientWidth(800),
	mClientHeight(600),
	mWindowTitle(L"D3D11 Application"),
	mD3DDriverType(D3D_DRIVER_TYPE_HARDWARE),
	mEnableMultisample(false),
	mMainWindow(0),
	mAppPaused(false),
	mMinimized(false),
	mMaximized(false),
	mMultisampleQuality(0),
	mDevice(0),
	mImmediateContext(0),
	mSwapChain(0),
	mDepthStencilBuffer(0),
	mRenderTargetView(0),
	mDepthStencilView(0)
{
	ZeroMemory(&mViewport, sizeof(D3D11_VIEWPORT));
	gD3DApp = this;
}

D3DApp::~D3DApp()
{
	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mSwapChain);
	ReleaseCOM(mDepthStencilBuffer);

	if (mImmediateContext)
	{
		mImmediateContext->ClearState();
	}

	ReleaseCOM(mImmediateContext);
	ReleaseCOM(mDevice);
}

float D3DApp::AspectRatio() const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

int D3DApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				UpdateScene(mTimer.DeltaTime());
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;
	}

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
	{
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (mDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{
				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;
	}

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
	{
		mAppPaused = true;
		mTimer.Stop();
		return 0;
	}

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
	{
		mAppPaused = false;
		mTimer.Start();
		OnResize();
		return 0;
	}

	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
	{
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);
	}

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
	{
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;
	}

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;

	case WM_CHAR:
		OnKeyDown(wParam, lParam);
		return 0;
	}


	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::Init()
{
	if (!InitMainWindow())
	{
		return false;
	}

	if (!InitDirect3D())
	{
		return false;
	}
	
	return true;
}

bool D3DApp::InitMainWindow()
{
	WNDCLASS WindowClass;
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindProcess;
	WindowClass.cbClsExtra = 0;
	WindowClass.cbWndExtra = 0;
	WindowClass.hInstance = mAppInstance;
	WindowClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
	WindowClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	WindowClass.lpszMenuName = 0;
	WindowClass.lpszClassName = L"D3DWindowClassName";

	if (!RegisterClass(&WindowClass))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	RECT WindowRect = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, false);

	int width = WindowRect.right - WindowRect.left;
	int height = WindowRect.bottom - WindowRect.top;

	mMainWindow = CreateWindow(L"D3DWindowClassName", mWindowTitle.c_str(), WS_OVERLAPPEDWINDOW, 
		CW_USEDEFAULT, CW_USEDEFAULT, width, height, 
		0, 0, mAppInstance, 0);

	if (!mMainWindow)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mMainWindow, SW_SHOW);
	UpdateWindow(mMainWindow);

	return true;
}

bool D3DApp::InitDirect3D()
{
	/* -----------------------------------------
	Create the D3D11 Device and DeviceContext
	----------------------------------------- */

	/*  -----------------------
	//? D3D11CreateDevice Notes
	-----------------------

	//! Function Signature

	HRESULT result = D3D11CreateDevice(
	pAdapter,
	DriverType,
	Software,
	Flags,
	pFeatureLevels,
	FeatureLevels,
	SDKVersion,
	ppDevice,
	pFeatureLevel,
	ppImmediateContext
	);

	//! Parameter Descriptions

	// The display adapter we want the created device to represent
	// Notes: Specifying "null" indicates the primary display adapter.
	//! IDXGIAdapter* pAdapter;

	// Type of driver our device will represent
	// Notes: In general, this should always be D3D_DRIVER_TYPE_HARDWARE, unless you need a reference, software, or WARP device.
	//! D3D_DRIVER_TYPE DriverType;

	// Software Driver
	// Notes: This is "null" if using a hardware driver.
	//! HMODULE Software;

	// (Optional) Device creation flags
	// Ex: D3D11_CREATE_DEVICE_DEBUG - Enables the Debug Layer, Direct3D will send debug messages to the VC++ output window.
	// Ex: D3D11_CREATE_DEVICE_SINGLETHREADED - Improves performance if you guarantee that Direct3D will not be called from multiple threads.
	//! UINT Flags;

	// Array of D3D feature levels
	// Notes: The order indicates the order in which to check for feature level support.
	// Specifying "null" indicates to choose the greatest feature level supported.
	//! const D3D_FEATURE_LEVEL* pFeatureLevels;

	// Size of pFeatureLevels array.
	// Notes: Specify "0" if you specified "null" for pFeatureLevels.
	//! UINT FeatureLevels;

	// Direct3D SDK Version
	// Notes: While studying this book, always use D3D11_SDK_VERSION.
	//! UINT SDKVersion;

	// Returns: Pointer to the device which this function will create
	//! ID3D11Device** ppDevice;

	// Returns: The first (highest) supported feature level from pFeatureLevels
	//! D3D_FEATURE_LEVEL* pFeatureLevel;

	// Returns: The created device context
	//! ID3D11DeviceContext** ppImmediateContext;

	*/

	UINT CreateDeviceFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	CreateDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL FeatureLevel;

	HRESULT CreateDeviceResult = D3D11CreateDevice(
		nullptr,
		mD3DDriverType,
		nullptr,
		CreateDeviceFlags,
		nullptr, 0,
		D3D11_SDK_VERSION,
		&mDevice,
		&FeatureLevel,
		&mImmediateContext
		);

	if (FAILED(CreateDeviceResult)) {
		MessageBox(0, L"D3D11CreateDevice failed.", 0, 0);
		return false;
	}

	if (FeatureLevel != D3D_FEATURE_LEVEL_11_0) {
		MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	/* -----------------------------------
	Check 4X MSAA Quality Level Support
	----------------------------------- */

	HR(mDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &mMultisampleQuality));
	assert(mMultisampleQuality > 0);

	/* ----------------------------------------------
	Describe the characteristics of the swap chain
	---------------------------------------------- */

	/*  --------------------------
	//? DXGI_SWAP_CHAIN_DESC Notes
	--------------------------

	//! Structure Signature

	typedef struct DXGI_SWAP_CHAIN_DESC {
	DXGI_MODE_DESC   BufferDesc;
	DXGI_SAMPLE_DESC SampleDesc;
	DXGI_USAGE       BufferUsage;
	UINT             BufferCount;
	HWND             OutputWindow;
	BOOL             Windowed;
	DXGI_SWAP_EFFECT SwapEffect;
	UINT             Flags;
	} DXGI_SWAP_CHAIN_DESC;

	//! Member Descriptions

	// Properties of the back buffer
	// Notes: The main properties that concern us are the width, height, and pixel format.
	//! DXGI_MODE_DESC BufferDesc;

	// Number of multisamples and quality level
	//! DXGI_SAMPLE_DESC SampleDesc;

	// Usage of the back buffer
	// Notes: Specify DXGI_USAGE_RENDER_TARGET_OUTPUT to indicate that the back buffer will be used for rendering.
	//! DXGI_USAGE BufferUsage;

	// Number of back buffers in the swap chain
	// Notes: Use "1" to allocate one back buffer (double buffering)
	//! UINT BufferCount;

	// Handle to the window we are rendering to
	//! HWND OutputWindow;

	// Notes: "true" indicates windowed-mode, "false" indicates full-screen mode
	//! BOOL Windowed;

	// Notes: Specify DXGI_SWAP_EFFECT_DISCARD to let the display driver select the most efficient presentation method
	//! DXGI_SWAP_EFFECT SwapEffect;

	// (Optional) flags
	// Notes: If you use DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH, then when you switch to full-screen mode, the application
	// will select a display mode that best matches the current back buffer settings.
	//! UINT Flags;

	*/

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;

	/*  --------------------------
	//? DXGI_DESC_MODE Notes
	--------------------------

	//! Structure Signature

	typedef struct DXGI_MODE_DESC {
	UINT                     Width;
	UINT                     Height;
	DXGI_RATIONAL            RefreshRate;
	DXGI_FORMAT              Format;
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	DXGI_MODE_SCALING        Scaling;
	} DXGI_MODE_DESC;

	//! Member Descriptions

	// Window resolution width
	//! UINT Width;

	// Window resolution height
	//! UINT Height;

	// Refresh rate defined in Hz (Numerator and Denominator)
	//! DXGI_RATIONAL RefreshRate;

	// Display format
	//! DXGI_FORMAT Format;

	// Scanline Drawing Mode
	//! DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;

	// Scaling Mode
	// Notes: Indicates how an image is stretched to fit the screen resolution.
	//! DXGI_MODE_SCALING Scaling;

	*/

	DXGI_MODE_DESC BackBufferDesc;
	BackBufferDesc.Width = 800;
	BackBufferDesc.Height = 600;
	// Most monitors cannot output more than 24-bit color, so extra precision would be wasted
	// Even though the monitor cannot output the 8-bit alpha, those bits can be used for other things like effects
	BackBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	BackBufferDesc.RefreshRate.Numerator = 60;
	BackBufferDesc.RefreshRate.Denominator = 1;
	BackBufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	BackBufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	SwapChainDesc.BufferDesc = BackBufferDesc;

	if (mEnableMultisample) {
		SwapChainDesc.SampleDesc.Count = 4;
		SwapChainDesc.SampleDesc.Quality = mMultisampleQuality - 1;
	}
	else {
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
	}

	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.OutputWindow = mMainWindow;
	SwapChainDesc.Windowed = true;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = 0;

	/* ---------------------
	Create the swap chain
	--------------------- */

	IDXGIDevice* dxgiDevice = 0;
	mDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

	IDXGIAdapter* dxgiAdapter = 0;
	dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

	IDXGIFactory* dxgiFactory = 0;
	dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

	dxgiFactory->CreateSwapChain(mDevice, &SwapChainDesc, &mSwapChain);

	ReleaseCOM(dxgiDevice);
	ReleaseCOM(dxgiAdapter);
	ReleaseCOM(dxgiFactory);

	OnResize();

	return true;
}

void D3DApp::OnResize()
{
	assert(mSwapChain);
	assert(mDevice);
	assert(mImmediateContext);

	ReleaseCOM(mRenderTargetView);
	ReleaseCOM(mDepthStencilView);
	ReleaseCOM(mDepthStencilBuffer);

	HR(mSwapChain->ResizeBuffers(1, mClientWidth, mClientHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));

	/* -----------------------------
	Create the render target view
	----------------------------- */

	ID3D11Texture2D* BackBuffer;

	mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer));
	mDevice->CreateRenderTargetView(BackBuffer, nullptr, &mRenderTargetView);

	ReleaseCOM(BackBuffer);

	/* -----------------------------------------------------
	Create the depth/stencil buffer and depth/stenci view
	----------------------------------------------------- */
	/*  --------------------------
	//? D3D11_TEXTURE2D_DESC Notes
	--------------------------

	//! Structure Signature

	typedef struct D3D11_TEXTURE2D_DESC {
	UINT             Width;
	UINT             Height;
	UINT             MipLevels;
	UINT             ArraySize;
	DXGI_FORMAT      Format;
	DXGI_SAMPLE_DESC SampleDesc;
	D3D11_USAGE      Usage;
	UINT             BindFlags;
	UINT             CPUAccessFlags;
	UINT             MiscFlags;
	} D3D11_TEXTURE2D_DESC;

	//! Member Descriptions

	// Width of the texture in texels
	//! UINT Width;

	// Height of the texture in texels
	//! UINT Height;

	// Number of mipmap levels
	// Notes: For the depth/stencil buffer, we only need one mipmap level
	//! UINT MipLevels;

	// Number of textures in a texture array
	// Notes: For the depth/stencil buffer, we only need one texture
	//! UINT ArraySize;

	// Depth format of the texels
	//! DXGI_FORMAT Format;

	// Number of multisamples and quality level
	// Notes: These must match the settings of the back buffer
	//! DXGI_SAMPLE_DESC SampleDesc;

	// How the texture will be used
	// Notes: The four usage types are
	// D3D11_USAGE_DEFAULT - The GPU will be reading/writing to this resource. The CPU will have no access.
	// D3D11_USAGE_IMMUTABLE - This resource is read-only for th GPU. The CPU will have no access.
	// D3D11_USAGE_DYNAMIC - The CPU needs to update the data contents frequently (per frame) and the GPU will read the data
	// D3D11_USAGE_STAGING - The CPU will be able to read the contents of the resource
	//! D3D11_USAGE Usage;

	// Where the resource will be bound to the pipeline
	//! UINT BindFlags;

	// How the CPU will access the resource (Read / Write)
	//! UINT CPUAccessFlags;

	// (Optional) flags
	//! UINT MiscFlags;

	*/

	D3D11_TEXTURE2D_DESC DepthStencilDesc;

	DepthStencilDesc.Width = 800;
	DepthStencilDesc.Height = 600;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	if (mEnableMultisample) {
		DepthStencilDesc.SampleDesc.Count = 4;
		DepthStencilDesc.SampleDesc.Quality = mMultisampleQuality - 1;
	}
	else {
		DepthStencilDesc.SampleDesc.Count = 1;
		DepthStencilDesc.SampleDesc.Quality = 0;
	}

	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.CPUAccessFlags = 0;
	DepthStencilDesc.MiscFlags = 0;

	mDevice->CreateTexture2D(&DepthStencilDesc, nullptr, &mDepthStencilBuffer);
	mDevice->CreateDepthStencilView(mDepthStencilBuffer, nullptr, &mDepthStencilView);

	/* -----------------------------------------
	Bind the views to the output merger stage
	----------------------------------------- */

	mImmediateContext->OMSetRenderTargets(1, &mRenderTargetView, mDepthStencilView);

	/* ----------------
	Set the viewport
	---------------- */

	mViewport.TopLeftX = 0.0f;
	mViewport.TopLeftY = 0.0f;
	mViewport.Width = 800.0f;
	mViewport.Height = 600.0f;
	mViewport.MinDepth = 0.0f;
	mViewport.MaxDepth = 1.0f;

	mImmediateContext->RSSetViewports(1, &mViewport);
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCount = 0;
	static float timeElapsed = 0.0f;

	frameCount++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCount;
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);
		outs << mWindowTitle << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";
		SetWindowText(mMainWindow, outs.str().c_str());

		// Reset for next average.
		frameCount = 0;
		timeElapsed += 1.0f;
	}
}
