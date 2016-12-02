/*  =======================
	Summary: 
	=======================  */

#ifndef GWAVE_H
#define GWAVE_H

#include "GObject.h"
#include "Waves.h"
#include "MathHelper.h"
#include "D3DUtil.h"

__declspec(align(16))
class GWave : public GObject
{
public:
	GWave();
	~GWave();

	void* operator new(size_t i) { return _mm_malloc(i,16);	}
	void operator delete(void* p) { _mm_free(p); }

	void Update(float currentTime, float dt, void* data);

private:
	Waves mWaves;

	DirectX::XMFLOAT2 mWaterTexOffset;
};

#endif // GWAVE_H