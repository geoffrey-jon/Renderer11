/*  =======================
	Summary:
	=======================  */

#ifndef GPLANEYZ_H
#define GPLANEYZ_H

#include "GObject.h"

__declspec(align(16))
class GPlaneYZ : public GObject
{
public:
	GPlaneYZ();
	GPlaneYZ(float height, float depth, UINT m, UINT n);

	~GPlaneYZ();

	void* operator new(size_t i){ return _mm_malloc(i,16); }
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GPLANEYZ_H