#include "libutils/lib_math.h"

int parseInt(const char* str, unsigned char width, bool numberIsString) {
	int num = 0;
	bool isNegative = false;
	unsigned char charOffset = (numberIsString?CHAR0:0);
	if (str[0]=='-') isNegative=true;
	else num = (str[0]-charOffset);

	for ( int i=1; i<width; i++ ) {
		num *= 10;
		num += (str[i] - charOffset);
	}
	if (isNegative) num *= -1;
	return num;
}

uint quantizeUFloat(float normalizedFloat) {
	// clamp to [0.0f, 1.0f]
	if (normalizedFloat < 0) normalizedFloat = 0.0f;
	else if (normalizedFloat > 1) normalizedFloat = 1.0f;

	return (int)(normalizedFloat * 255.0f + 0.5f);
}

int quantizeFloat(float normalizedFloat) {
	// clamp to [-1.0f, 1.0f]
	if (normalizedFloat < -1.0f) normalizedFloat = -1.0f;
	else if (normalizedFloat > 1.0f) normalizedFloat = 1.0f;

	return (int)(normalizedFloat * 127.0f + (normalizedFloat > 0.0f ? 0.5f : -0.5f));
}

float dequantize(int quantized) {
	return ((float)quantized - (quantized > 0 ? 0.5f : -0.5f)) / 127.0f;
}

float dequantizeU(uint quantized) {
	return ((float)quantized - 0.5f) / 255.0f;
}
