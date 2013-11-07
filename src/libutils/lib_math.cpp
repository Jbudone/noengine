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

// Blatantly copied from source
// http://freespace.virgin.net/hugo.elias/models/m_perlin.htm
// float randomFloat(int x) {
//     x = (x<<13) ^ x;
//     return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 7fffffff) / 1073741824.0);  
// }
float randomFloat(int x, int y) {
    int n = x + y * 57;
	// return randomFloat(n);
    // n = pow((n<<13), n);
	n = (n<<13) ^ n;
    return ( 1.0 - (float)( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / (float)1073741824.0);  
}

float interpolateLinear(float v0, float v1, float x) {
	return v0 * ( 1 - x ) + v1 * x;
}

float interpolateCosine(float v0, float v1, float x) {
	x = ( 1 - cos(PI*x) ) * 0.5f;
	return v0 * ( 1 - x ) + v1 * x;
}

float interpolateCubic(float v0, float v1, float v2, float v3, float x) {
	float P = (v3 - v2) - (v0 - v1);
	float Q = (v0 - v1) - P;
	float R = v2 - v0;
	float S = v1;

	return P*x*x*x + Q*x*x + R*x + S;
}

float smoothNoise(float x, float y) {
	float corners = ( randomFloat(x-1,y-1) +
					  randomFloat(x-1,y+1) +
					  randomFloat(x+1,y-1) +
					  randomFloat(x+1,y+1) ) / 16;
	float sides   = ( randomFloat(x-1,y) +
					  randomFloat(x+1,y) +
					  randomFloat(x,y-1) +
					  randomFloat(x,y+1) ) / 8;
	float center  = randomFloat(x,y) / 4;
	return corners + sides + center;
}

float interpolatedNoise(float x, float y) {
	
	float v0 = smoothNoise( (int)x, (int)y ),
		  v1 = smoothNoise( (int)x + 1, (int)y ),
		  v2 = smoothNoise( (int)x, (int)y + 1 ),
		  v3 = smoothNoise( (int)x + 1, (int)y + 1 );

	float i0 = interpolateCosine( v0, v1, x - (float)((int)x) ),
		  i1 = interpolateCosine( v2, v3, x - (float)((int)x) );

	return interpolateCosine( i0, i1, y - (float)((int)y) );
}

float perlinNoise(float x, float y, float persistence, float octaves) {
	
	float total = 0.0f;

	int frequency, amplitude;
	for ( int i = 0; i < octaves; ++i ) {
		frequency = pow(2, i);
		amplitude = pow(persistence, i);

		total += interpolatedNoise( x * frequency, y * frequency ) * amplitude;
	}

	return total;
}
