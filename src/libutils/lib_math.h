#ifndef __LIB_MATH_H__
#define __LIB_MATH_H__


/*
 * Math
 *
 * 		Mathy things
 *
 * TODO
 *
 *  - ...
 *
 ***/

#define CHAR0 48 // ASCII dec code for 0
#define PI 3.1415926
#include <cmath>
#include "typeinfo/t_types.h"

extern "C" {
	int parseInt(const char* str, unsigned char width, bool numberIsString = true);
	uint quantizeUFloat(float normalizedFloat);
	int quantizeFloat(float normalizedFloat);
	float dequantize(int quantized);
	float dequantizeU(uint quantized);
	// float randomFloat(int x); // returns f[-1,1] using seed x
	float randomFloat(int x, int y); // returns f[-1,1] using seed (x,y)
	float interpolateLinear(float v0, float v1, float x);
	float interpolateCosine(float v0, float v1, float x);
	float interpolateCubic( float v0, float v1, float v2, float v3, float x);
	float smoothNoise(float x, float y);
	float interpolatedNoise(float x, float y);
	float perlinNoise(float x, float y, float persistence, float octaves);
}


#endif
