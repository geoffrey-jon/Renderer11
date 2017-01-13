/*  =======================
	Summary: 
	=======================  */

#ifndef GSPHERE_H
#define GSPHERE_H

#include "GObject.h"
#include "GeometryGenerator.h"

__declspec(align(16))
class GSphere : public GObject
{
public:
	GSphere();
	~GSphere();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GSPHERE_H