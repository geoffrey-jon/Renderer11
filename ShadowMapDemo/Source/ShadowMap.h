#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include "D3D11.h"

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width, UINT height);
	~ShadowMap();

	ID3D11ShaderResourceView* GetDepthMapSRV();
	ID3D11DepthStencilView* GetDepthMapDSV();
	D3D11_VIEWPORT GetViewport();

private:
	ShadowMap(const ShadowMap& rhs);
	ShadowMap& operator=(const ShadowMap& rhs);

private:
	UINT mWidth;
	UINT mHeight;

	ID3D11ShaderResourceView* mDepthMapSRV;
	ID3D11DepthStencilView* mDepthMapDSV;

	D3D11_VIEWPORT mViewport;
};

#endif // SHADOWMAP_H