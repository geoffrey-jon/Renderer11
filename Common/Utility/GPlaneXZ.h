/*  =======================
	Summary:
	=======================  */

#ifndef GPLANEXZ_H
#define GPLANEXZ_H

#include "GObject.h"

__declspec(align(16))
class GPlaneXZ : public GObject
{
public:
	GPlaneXZ();
	GPlaneXZ(float width, float depth, UINT m, UINT n);

	~GPlaneXZ();

	void CreatePlane(float width, float depth, UINT m, UINT n);

	void* operator new(size_t i){ return _mm_malloc(i,16); }
	void operator delete(void* p) { _mm_free(p); }

private:

};

#endif // GPLANEXZ_H