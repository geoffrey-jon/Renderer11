/*  =======================
Summary:
=======================  */

#ifndef GPLANE_H
#define GPLANE_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GPlane : public GObject
{
public:
	GPlane();
	~GPlane();

	void* operator new(size_t i){ return _mm_malloc(i,16); }
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GPLANE_H