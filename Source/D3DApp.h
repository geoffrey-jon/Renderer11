/*  =============================================
	Summary: Direct3D Application Framework Class
	=============================================  */

#ifndef D3DAPP_H
#define D3DAPP_H

#include "D3D11.h"
#include "D3DUtil.h"
#include "GameTimer.h"

#include <Windows.h>
#include <WindowsX.h>
#include <assert.h>
#include <string>
#include <sstream>

class D3DApp
{
public:
	D3DApp(HINSTANCE Instance);
	virtual ~D3DApp();

	float AspectRatio() const;

	int Run();

	// Framework methods.  Derived client class overrides these methods to 
	// implement specific application requirements.

	virtual bool Init();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	virtual void OnResize();

	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

	virtual void OnKeyDown(WPARAM key, LPARAM info) { }

protected:
	bool InitMainWindow();
	bool InitDirect3D();
	void CalculateFrameStats();

protected:
	HINSTANCE mAppInstance;
	HWND mMainWindow;

	D3D_DRIVER_TYPE mD3DDriverType;

	ID3D11Device* mDevice;
	ID3D11DeviceContext* mImmediateContext;

	IDXGISwapChain* mSwapChain;

	ID3D11RenderTargetView* mRenderTargetView;
	ID3D11DepthStencilView* mDepthStencilView;

	ID3D11Texture2D* mDepthStencilBuffer;

	D3D11_VIEWPORT mViewport;

	GameTimer mTimer;

	bool mAppPaused;
	bool mMinimized;
	bool mMaximized;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mWindowTitle;

	int mClientWidth;
	int mClientHeight;

	bool mEnableMultisample;
	UINT mMultisampleQuality;
};

#endif // D3DAPP_H