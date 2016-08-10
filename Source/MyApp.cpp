/*  =======================
	Summary: DX11 Chapter 4
	=======================  */

#include "MyApp.h"

MyApp::MyApp(HINSTANCE Instance) :
	D3DApp(Instance)
{
	mWindowTitle = L"D3D Initialization";
}

MyApp::~MyApp()
{
}

bool MyApp::Init()
{
	if (!D3DApp::Init())
	{
		return false;
	}

	return true;
}

void MyApp::OnResize()
{
	D3DApp::OnResize();
}

void MyApp::UpdateScene(float dt)
{
}

void MyApp::DrawScene()
{
	assert(mImmediateContext);
	assert(mSwapChain);

	mImmediateContext->ClearRenderTargetView(mRenderTargetView, reinterpret_cast<const float*>(&Colors::LightSteelBlue));
	mImmediateContext->ClearDepthStencilView(mDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(mSwapChain->Present(0, 0));
}