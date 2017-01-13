/*  =======================
	Summary: 
	=======================  */

#ifndef GSKY_H
#define GSKY_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GSky : public GObject
{
public:
	GSky(float skySphereRadius);
	~GSky();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

	void SetEyePos(float x, float y, float z);

private:
};

#endif // GSKY_H