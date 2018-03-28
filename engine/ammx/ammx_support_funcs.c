/**
	\file bmpcache.cpp
	\brief LightEngine 3D: Bitmap cache management
	\brief All platforms implementation
	\author Frederic Meslin (fred@fredslab.net)
	\twitter @marzacdev
	\website http://fredslab.net
	\copyright Frederic Meslin 2015 - 2018
	\version 1.5

	The MIT License (MIT)
	Copyright (c) 2015-2018 Fr�d�ric Meslin

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

#include "ammx_support_funcs.h"

void prepare_pmul88(__reg("a0") uint8_t* c) = ""
"\tLOAD #$10000000,e0\n"
"\tLOAD (a0),e2\n"
"\tLOAD #$3,e7\n"
"\tvperm #$48494a4b,d0,e2,e5\n";

void pmul88(__reg("a0") uint8_t* p, __reg("a1") uint8_t* t) = ""
"\tLOAD (a1),e1\n"
"\tvperm #$48494a4b,e7,e1,e4\n"
"\tvperm #$4c4d4e4f,e7,e1,e0\n"
"\tpmul88 e4,e5,e3\n"
"\tpmul88 e0,e5,e1\n"
"\tvperm #$9bdf1357,e1,e3,e6\n"
"\tstore e6,(a0)+\n";

__reg("d0") int32_t divsl(__reg("d1") int q) = ""
"\tmove.l #$10000000,d0\n"
"\tdivs.l d1,d0\n"
;

void ammx_fill_flat_tex_zc(short d, int u1, int v1, int w1, int au, int av, int aw, uint32_t texMaskU, uint32_t texMaskV, uint32_t texSizeU, uint8_t* p, uint8_t* c, uint32_t* texPixels) {
	prepare_pmul88(c);
    while (d>=0) {
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t) (u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t) (v1 * z)) & texMaskV;
		// uint32_t tu = ((u1 * z) >> 24) & texMaskU;
		// uint32_t tv = ((v1 * z) >> 24) & texMaskV;
		uint8_t * t = (uint8_t *) &texPixels[tu + (tv << texSizeU)];

		pmul88(p, t);
		// p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;

		d -= 2;
	}
}