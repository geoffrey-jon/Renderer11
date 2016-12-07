/*  =======================
	Summary:
	=======================  */

#ifndef GPLANEXY_H
#define GPLANEXY_H

#include "GObject.h"

__declspec(align(16))
class GPlaneXY : public GObject
{
public:
	GPlaneXY();
	GPlaneXY(float width, float height, UINT m, UINT n);

	~GPlaneXY();

	void* operator new(size_t i){ return _mm_malloc(i,16); }
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GPLANEXY_H