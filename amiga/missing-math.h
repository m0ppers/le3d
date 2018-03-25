#ifndef MISSING_MATH_H
#define MISSING_MATH_H

extern "C" {
    // for some reasons there are no includes for these in the toolchain
    // but their symbols are defined anyway in the stdlib...MAGIC!
    float cosf(float);
    float sinf(float);
	float floorf(float);
	float sqrtf(float);
	float copysignf(float,float);
	float ceilf(float);
	float tanf(float);
}

#endif