/*  ======================
	Summary: Picking Demo
	======================  */

#ifndef RENDERSTATES_H
#define RENDERSTATES_H

#include "D3DUtil.h"

class RenderStates
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ID3D11RasterizerState* DefaultRS;
	static ID3D11DepthStencilState* DefaultDSS;
	static ID3D11BlendState* DefaultBS;
	static ID3D11SamplerState* DefaultSS;

	static ID3D11RasterizerState* NoCullRS;
	static ID3D11DepthStencilState* LessEqualDSS;
};

#endif // RENDERSTATES_H