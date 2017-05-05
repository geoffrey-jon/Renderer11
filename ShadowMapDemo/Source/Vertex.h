#ifndef VERTEX_H
#define VERTEX_H

#include <DirectXMath.h>

struct Vertex
{
	Vertex() : Pos(0.0f, 0.0f, 0.0f), Normal(0.0f, 0.0f, 0.0f), Tex(0.0f, 0.0f), TangentU(0.0f, 0.0f, 0.0f) {}
	Vertex(float px, float py, float pz, float nx, float ny, float nz, float u, float v, float tx, float ty, float tz)
		: Pos(px, py, pz), Normal(nx, ny, nz), Tex(u, v), TangentU(tx, ty, tz) {}

	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT2 Tex;
	DirectX::XMFLOAT3 TangentU;
};

struct VertexPosition
{
	VertexPosition() : Pos(0.0f, 0.0f, 0.0f) {}
	VertexPosition(float px, float py, float pz) : Pos(px, py, pz) {}

	DirectX::XMFLOAT3 Pos;
};

#endif // VERTEX_H