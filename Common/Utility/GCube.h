/*  =======================
	Summary: 
	=======================  */

#ifndef GCUBE_H
#define GCUBE_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GCube : public GObject
{
public:
	GCube();
	~GCube();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GCUBE_H