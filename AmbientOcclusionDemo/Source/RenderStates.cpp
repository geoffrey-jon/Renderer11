/*  ======================
	Summary: Picking Demo
	======================  */

#include "RenderStates.h"

ID3D11RasterizerState* RenderStates::DefaultRS = 0;
ID3D11DepthStencilState* RenderStates::DefaultDSS = 0;
ID3D11BlendState* RenderStates::DefaultBS = 0;
ID3D11SamplerState* RenderStates::DefaultSS = 0;

ID3D11RasterizerState* RenderStates::NoCullRS = 0;
ID3D11RasterizerState* RenderStates::ShadowMapRS = 0;

ID3D11SamplerState* RenderStates::ShadowMapCompSS = 0;
ID3D11SamplerState* RenderStates::ShadowMapSS = 0;
ID3D11SamplerState* RenderStates::SsaoSS = 0;

ID3D11DepthStencilState* RenderStates::LessEqualDSS = 0;

void RenderStates::InitAll(ID3D11Device* device)
{
	// Default Rasterizer State
	D3D11_RASTERIZER_DESC DefaultRSDesc;
	DefaultRSDesc.FillMode = D3D11_FILL_SOLID;
	DefaultRSDesc.CullMode = D3D11_CULL_BACK;
	DefaultRSDesc.FrontCounterClockwise = false;
	DefaultRSDesc.DepthBias = 0;
	DefaultRSDesc.DepthBiasClamp = 0.0f;
	DefaultRSDesc.SlopeScaledDepthBias = 0.0f;
	DefaultRSDesc.DepthClipEnable = true;
	DefaultRSDesc.ScissorEnable = false;
	DefaultRSDesc.MultisampleEnable = false;
	DefaultRSDesc.AntialiasedLineEnable = false;
	HR(device->CreateRasterizerState(&DefaultRSDesc, &DefaultRS));

	// No Cull Rasterizer State
	D3D11_RASTERIZER_DESC NoCullRSDesc;
	NoCullRSDesc.FillMode = D3D11_FILL_SOLID;
	NoCullRSDesc.CullMode = D3D11_CULL_NONE;
	NoCullRSDesc.FrontCounterClockwise = false;
	NoCullRSDesc.DepthBias = 0;
	NoCullRSDesc.DepthBiasClamp = 0.0f;
	NoCullRSDesc.SlopeScaledDepthBias = 0.0f;
	NoCullRSDesc.DepthClipEnable = true;
	NoCullRSDesc.ScissorEnable = false;
	NoCullRSDesc.MultisampleEnable = false;
	NoCullRSDesc.AntialiasedLineEnable = false;
	HR(device->CreateRasterizerState(&NoCullRSDesc, &NoCullRS));

	// Default Rasterizer State
	D3D11_RASTERIZER_DESC ShadowMapRSDesc;
	ShadowMapRSDesc.FillMode = D3D11_FILL_SOLID;
	ShadowMapRSDesc.CullMode = D3D11_CULL_BACK;
	ShadowMapRSDesc.FrontCounterClockwise = false;
	ShadowMapRSDesc.DepthBias = 100000;
	ShadowMapRSDesc.DepthBiasClamp = 0.0f;
	ShadowMapRSDesc.SlopeScaledDepthBias = 0.0f;
	ShadowMapRSDesc.DepthClipEnable = true;
	ShadowMapRSDesc.ScissorEnable = false;
	ShadowMapRSDesc.MultisampleEnable = false;
	ShadowMapRSDesc.AntialiasedLineEnable = false;
	HR(device->CreateRasterizerState(&ShadowMapRSDesc, &ShadowMapRS));

	// Default Depth/Stencil State
	D3D11_DEPTH_STENCIL_DESC DefaultDSSDesc;
	DefaultDSSDesc.DepthEnable = true;
	DefaultDSSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DefaultDSSDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DefaultDSSDesc.StencilEnable = false;
	DefaultDSSDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	DefaultDSSDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

	DefaultDSSDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DefaultDSSDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DefaultDSSDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DefaultDSSDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	DefaultDSSDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	DefaultDSSDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	DefaultDSSDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	DefaultDSSDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	
	HR(device->CreateDepthStencilState(&DefaultDSSDesc, &DefaultDSS));

	// Default Blend State
	D3D11_BLEND_DESC DefaultBSDesc;
	DefaultBSDesc.AlphaToCoverageEnable = false;
	DefaultBSDesc.IndependentBlendEnable = false;
	DefaultBSDesc.RenderTarget[0].BlendEnable = false;
	DefaultBSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	DefaultBSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	DefaultBSDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	DefaultBSDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	DefaultBSDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	DefaultBSDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	DefaultBSDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HR(device->CreateBlendState(&DefaultBSDesc, &DefaultBS));

	// Default Sampler State
	D3D11_SAMPLER_DESC DefaultSSDesc;
	DefaultSSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	DefaultSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	DefaultSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	DefaultSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	DefaultSSDesc.MipLODBias = 0.0f;
	DefaultSSDesc.MaxAnisotropy = 1;
	DefaultSSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	DefaultSSDesc.BorderColor[0] = 1.0f;
	DefaultSSDesc.BorderColor[1] = 1.0f;
	DefaultSSDesc.BorderColor[2] = 1.0f;
	DefaultSSDesc.BorderColor[3] = 1.0f;
	DefaultSSDesc.MinLOD = -FLT_MAX;
	DefaultSSDesc.MaxLOD = FLT_MAX;

	HR(device->CreateSamplerState(&DefaultSSDesc, &DefaultSS));

	// Shadow Map Comparison Sampler State
	D3D11_SAMPLER_DESC ShadowMapCompSSDesc;
	ShadowMapCompSSDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	ShadowMapCompSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapCompSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapCompSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapCompSSDesc.MipLODBias = 0.0f;
	ShadowMapCompSSDesc.MaxAnisotropy = 1;
	ShadowMapCompSSDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	ShadowMapCompSSDesc.BorderColor[0] = 0.0f;
	ShadowMapCompSSDesc.BorderColor[1] = 0.0f;
	ShadowMapCompSSDesc.BorderColor[2] = 0.0f;
	ShadowMapCompSSDesc.BorderColor[3] = 0.0f;
	ShadowMapCompSSDesc.MinLOD = -FLT_MAX;
	ShadowMapCompSSDesc.MaxLOD = FLT_MAX;

	HR(device->CreateSamplerState(&ShadowMapCompSSDesc, &ShadowMapCompSS));

	// Shadow Map Sampler State
	D3D11_SAMPLER_DESC ShadowMapSSDesc;
	ShadowMapSSDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	ShadowMapSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	ShadowMapSSDesc.MipLODBias = 0.0f;
	ShadowMapSSDesc.MaxAnisotropy = 1;
	ShadowMapSSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ShadowMapSSDesc.BorderColor[0] = 0.0f;
	ShadowMapSSDesc.BorderColor[1] = 0.0f;
	ShadowMapSSDesc.BorderColor[2] = 0.0f;
	ShadowMapSSDesc.BorderColor[3] = 0.0f;
	ShadowMapSSDesc.MinLOD = -FLT_MAX;
	ShadowMapSSDesc.MaxLOD = FLT_MAX;

	HR(device->CreateSamplerState(&ShadowMapSSDesc, &ShadowMapSS));

	// SSAO Sampler State
	D3D11_SAMPLER_DESC SsaoSSDesc;
	SsaoSSDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	SsaoSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	SsaoSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	SsaoSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	SsaoSSDesc.MipLODBias = 0.0f;
	SsaoSSDesc.MaxAnisotropy = 1;
	SsaoSSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SsaoSSDesc.BorderColor[0] = 0.0f;
	SsaoSSDesc.BorderColor[1] = 0.0f;
	SsaoSSDesc.BorderColor[2] = 0.0f;
	SsaoSSDesc.BorderColor[3] = 1e5f;
	SsaoSSDesc.MinLOD = -FLT_MAX;
	SsaoSSDesc.MaxLOD = FLT_MAX;

	HR(device->CreateSamplerState(&SsaoSSDesc, &SsaoSS));

	// LessEqualDSS
	D3D11_DEPTH_STENCIL_DESC lessEqualDesc;
	lessEqualDesc.DepthEnable = true;
	lessEqualDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	lessEqualDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	lessEqualDesc.StencilEnable = false;

	HR(device->CreateDepthStencilState(&lessEqualDesc, &LessEqualDSS));
}

void RenderStates::DestroyAll()
{
	ReleaseCOM(DefaultRS);
	ReleaseCOM(DefaultDSS);
	ReleaseCOM(DefaultBS);
	ReleaseCOM(DefaultSS);
	ReleaseCOM(LessEqualDSS);
}
