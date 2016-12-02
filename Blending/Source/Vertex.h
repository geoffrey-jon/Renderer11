#ifndef VERTEX_H
#define VERTEX_H

#include <DirectXMath.h>

struct Vertex
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
};

#endif // VERTEX_H