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
#include "typeinfo/t_types.h"

extern "C" {
	int parseInt(const char* str, unsigned char width, bool numberIsString = true);
	uint quantizeUFloat(float normalizedFloat);
	int quantizeFloat(float normalizedFloat);
	float dequantize(int quantized);
	float dequantizeU(uint quantized);
}


#endif
