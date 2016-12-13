/*  =======================
	Summary:
	=======================  */

#ifndef GTRIANGLE_H
#define GTRIANGLE_H

#include "GObject.h"

__declspec(align(16))
class GTriangle : public GObject
{
public:
	GTriangle();
	GTriangle(Vertex v0, Vertex v1, Vertex v2);

	~GTriangle();

	void* operator new(size_t i){ return _mm_malloc(i,16); }
	void operator delete(void* p) { _mm_free(p); }

	void SetVertices(Vertex v0, Vertex v1, Vertex v2);

	void Update(void* data);

private:

};

#endif // GTRIANGLE_H