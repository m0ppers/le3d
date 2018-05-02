/**
	\file profiling.h
	\brief LightEngine 3D: Missing amiga math header files. Symbols are there
	\brief Just header files were missing
	\brief Amiga only
    \brief usage of these in the main code should NOT be commited!
	\author Andreas Streichardt (andreas@mop.koeln)
	\twitter @m0ppers
	\website https://mop.koeln/
	\copyright Andreas Streichardt 2018
	\version 1.5

	The MIT License (MIT)
	Copyright (c) 2015-2018 Frédéric Meslin

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

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
	float fabsf(float);
}

#endif