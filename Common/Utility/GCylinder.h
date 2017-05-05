/*  =======================
	Summary: 
	=======================  */

#ifndef GCYLINDER_H
#define GCYLINDER_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GCylinder : public GObject
{
public:
	GCylinder();
	~GCylinder();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GCYLINDER_H