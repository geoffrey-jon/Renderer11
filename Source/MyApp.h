/*  =======================
	Summary: DX11 Chapter 4 
	=======================  */

#ifndef MYAPP_H
#define MYAPP_H

#include "D3DApp.h"

class MyApp : public D3DApp
{
public:
	MyApp(HINSTANCE Instance);
	~MyApp();

	bool Init() override;
	void OnResize() override;

	void UpdateScene(float dt) override;
	void DrawScene() override;
};

#endif // MYAPP_H