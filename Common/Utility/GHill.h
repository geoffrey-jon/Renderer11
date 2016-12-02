/*  =======================
	Summary: 
	=======================  */

#ifndef GHILL_H
#define GHILL_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GHill : public GObject
{
public:
	GHill();
	~GHill();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

private:
	float GetHillHeight(float x, float z)const;
	DirectX::XMFLOAT3 GetHillNormal(float x, float z)const;
};

#endif // GHILL_H