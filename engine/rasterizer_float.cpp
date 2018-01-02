/**
	\file rasterizer_float.cpp
	\brief LightEngine 3D: Triangle rasterizer (floating point)
	\brief All platforms implementation
	\brief Support textured triangles
	\brief Textures can have an alpha channel and mipmaps
	\author Frederic Meslin (fred@fredslab.net)
	\twitter @marzacdev
	\website http://fredslab.net
	\copyright Frederic Meslin 2015 - 2018
	\version 1.4

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

#include "global.h"
#include "config.h"

#if LE_RENDERER_INTRASTER == 0

#include "rasterizer.h"
#include "draw.h"
#include "bmpcache.h"

#include <stdlib.h>
#include <strings.h>

/*****************************************************************************/
LeRasterizer::LeRasterizer(int width, int height) :
	color(0xFFFFFF),
	bmp(NULL),
	texPixels(NULL),
	texSizeU(0), texSizeV(0),
	texMaskU(0), texMaskV(0),
	background(0)
{
	memset(xs, 0, sizeof(float) * 4);
	memset(ys, 0, sizeof(float) * 4);
	memset(ws, 0, sizeof(float) * 4);
	memset(us, 0, sizeof(float) * 4);
	memset(vs, 0, sizeof(float) * 4);

	frame.allocate(width, height + 2);
	frame.data = ((uint32_t *) frame.data) + frame.tx;
	frame.ty -= 2;
	frame.clear(0);
}

LeRasterizer::~LeRasterizer()
{
	frame.data = ((uint32_t *) frame.data) - frame.tx;
	frame.ty += 2;
	frame.deallocate();
}

/*****************************************************************************/
void LeRasterizer::flush()
{
	frame.clear(background);
}

void LeRasterizer::setBackground(uint32_t color)
{
	background = color;
}

/*****************************************************************************/
void LeRasterizer::rasterList(LeTriList * trilist)
{
	trilist->zSort();

	for (int i = 0; i < trilist->noValid; i++) {
		LeTriangle * tri = &trilist->triangles[trilist->srcIndices[i]];

	// Retrieve the material
		LeBmpCache::Slot * slot = &bmpCache.slots[tri->tex];
		if (slot->flags & LE_BMPCACHE_ANIMATION)
			bmp = &slot->extras[slot->cursor];
		else bmp = slot->bitmap;
		color = tri->color;

	// Convert position coordinates
		xs[0] = floorf(tri->xs[0]);
		xs[1] = floorf(tri->xs[1]);
		xs[2] = floorf(tri->xs[2]);
		ys[0] = floorf(tri->ys[0]);
		ys[1] = floorf(tri->ys[1]);
		ys[2] = floorf(tri->ys[2]);
		ws[0] = tri->zs[0];
		ws[1] = tri->zs[1];
		ws[2] = tri->zs[2];

	// Sort vertexes vertically
		int vt = 0, vb = 0, vm1 = 0, vm2 = 3;
		if (ys[0] < ys[1]) {
			if (ys[0] < ys[2]) {
				vt = 0;
				if (ys[1] < ys[2]) {vm1 = 1; vb = 2;}
				else {vm1 = 2; vb = 1;}
			}else{
				vt = 2;	vm1 = 0; vb = 1;
			}
		}else{
			if (ys[1] < ys[2]) {
				vt = 1;
				if (ys[0] < ys[2]) {vm1 = 0; vb = 2;}
				else {vm1 = 2; vb = 0;}
			}else{
				vt = 2; vm1 = 1; vb = 0;
			}
		}

	// Get vertical span
		float dy = ys[vb] - ys[vt];
		if (dy == 0.0f) continue;

	// Choose the mipmap level
	#if LE_RENDERER_MIPMAPS == 1
		if (bmp->mmLevels) {
			float utop = tri->us[vt] / tri->zs[vt];
			float ubot = tri->us[vb] / tri->zs[vb];
			float vtop = tri->vs[vt] / tri->zs[vt];
			float vbot = tri->vs[vb] / tri->zs[vb];
			float d = cmax(fabs(utop - ubot), fabs(vtop - vbot));

			int r = (d * bmp->ty + dy * 0.5f) / dy;
			int l = LeGlobal::log2i32(r);
			l = cmin(l, bmp->mmLevels - 1);
			bmp = bmp->mipmaps[l];
		}
	#endif

	// Retrieve texture information
		texPixels = (uint32_t *) bmp->data;
		texSizeU = bmp->txP2;
		texSizeV = bmp->tyP2;
		texMaskU = (1 << bmp->txP2) - 1;
		texMaskV = (1 << bmp->tyP2) - 1;

	#if LE_USE_SIMD == 1
		float texSizeUFloat = (float) (1 << texSizeU);
		texScale_4.v = (V4SF) {texSizeUFloat, texSizeUFloat, texSizeUFloat, texSizeUFloat};
		texMaskU_4.v = (V4SU) {texMaskU, texMaskU, texMaskU, texMaskU};
		texMaskV_4.v = (V4SU) {texMaskV << texSizeU, texMaskV << texSizeU, texMaskV << texSizeU, texMaskV << texSizeU};
		v4si zv = {0, 0, 0, 0};
		color_1.v = (V4SU) _mm_loadu_si128((__m128i *) &color);
		color_1.v = (V4SU) _mm_unpacklo_epi32((__m128i)color_1.v,(__m128i)color_1.v);
		color_1.v = (V4SU) _mm_unpacklo_epi8((__m128i)color_1.v, (__m128i)zv.v);
	#endif

	// Convert texture coordinates
		float sx = (float) (1 << bmp->txP2);
		us[0] = tri->us[0] * sx;
		us[1] = tri->us[1] * sx;
		us[2] = tri->us[2] * sx;

		float sy = (float) (1 << bmp->tyP2);
		vs[0] = tri->vs[0] * sy;
		vs[1] = tri->vs[1] * sy;
		vs[2] = tri->vs[2] * sy;

	// Compute the mean vertex
		float n = (ys[vm1] - ys[vt]) / dy;
		xs[3] = (xs[vb] - xs[vt]) * n + xs[vt];
		ys[3] = ys[vm1];
		ws[3] = (ws[vb] - ws[vt]) * n + ws[vt];
		us[3] = (us[vb] - us[vt]) * n + us[vt];
		vs[3] = (vs[vb] - vs[vt]) * n + vs[vt];

	// Sort vertexes horizontally
		int dx = xs[vm2] - xs[vm1];
		if (dx < 0) {int t = vm1; vm1 = vm2; vm2 = t;}

	// Render the triangle
		topTriangleZC(vt, vm1, vm2);
		bottomTriangleZC(vm1, vm2, vb);
	}
}

/*****************************************************************************/
void LeRasterizer::topTriangleZC(int vt, int vm1, int vm2)
{
	float d = ys[vm1] - ys[vt];
	if (d == 0.0f) return;

	float id = 1.0f / d;
	float ax1 = (xs[vm1] - xs[vt]) * id;
	float aw1 = (ws[vm1] - ws[vt]) * id;
	float au1 = (us[vm1] - us[vt]) * id;
	float av1 = (vs[vm1] - vs[vt]) * id;
	float ax2 = (xs[vm2] - xs[vt]) * id;
	float aw2 = (ws[vm2] - ws[vt]) * id;
	float au2 = (us[vm2] - us[vt]) * id;
	float av2 = (vs[vm2] - vs[vt]) * id;

	float x1 = xs[vt];
	float x2 = x1;
	float w1 = ws[vt];
	float w2 = w1;
	float u1 = us[vt];
	float u2 = u1;
	float v1 = vs[vt];
	float v2 = v1;

	int y1 = (int) ys[vt];
	int y2 = (int) ys[vm1];
	if (bmp->flags & LE_BMP_RGBA) {
		for (int y = y1; y < y2; y++) {
			fillFlatTexAlphaZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
			x1 += ax1; x2 += ax2;
			u1 += au1; u2 += au2;
			v1 += av1; v2 += av2;
			w2 += aw2; w1 += aw1;
		}
	}else{
		for (int y = y1; y < y2; y++) {
			fillFlatTexZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
			x1 += ax1; x2 += ax2;
			u1 += au1; u2 += au2;
			v1 += av1; v2 += av2;
			w2 += aw2; w1 += aw1;
		}
	}
}

void LeRasterizer::bottomTriangleZC(int vm1, int vm2, int vb)
{
	float d = ys[vb] - ys[vm1];
	if (d == 0.0f) return;

	float id = 1.0f / d;
	float ax1 = (xs[vb] - xs[vm1]) * id;
	float aw1 = (ws[vb] - ws[vm1]) * id;
	float au1 = (us[vb] - us[vm1]) * id;
	float av1 = (vs[vb] - vs[vm1]) * id;
	float ax2 = (xs[vb] - xs[vm2]) * id;
	float aw2 = (ws[vb] - ws[vm2]) * id;
	float au2 = (us[vb] - us[vm2]) * id;
	float av2 = (vs[vb] - vs[vm2]) * id;

	float x1 = xs[vm1];
	float w1 = ws[vm1];
	float u1 = us[vm1];
	float v1 = vs[vm1];

	float x2 = xs[vm2];
	float w2 = ws[vm2];
	float u2 = us[vm2];
	float v2 = vs[vm2];

	int y1 = (int) ys[vm1];
	int y2 = (int) ys[vb];

	if (bmp->flags & LE_BMP_RGBA) {
		for (int y = y1; y < y2; y++) {
			fillFlatTexAlphaZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
			x1 += ax1; x2 += ax2;
			w1 += aw1; w2 += aw2;
			u1 += au1; u2 += au2;
			v1 += av1; v2 += av2;
		}
	}else{
		for (int y = y1; y < y2; y++) {
			fillFlatTexZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
			x1 += ax1; x2 += ax2;
			w1 += aw1; w2 += aw2;
			u1 += au1; u2 += au2;
			v1 += av1; v2 += av2;
		}
	}
}

/*****************************************************************************/
#if LE_USE_SIMD == 1 && LE_USE_SSE2 == 1
inline void LeRasterizer::fillFlatTexZC(float y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
	float d = x2 - x1;
	if (d == 0.0f) return;

	float id = 1.0f / d;
	float au = (u2 - u1) * id;
	float av = (v2 - v1) * id;
	float aw = (w2 - w1) * id;

	v4sf u_4 = {u1, u1 + au, u1 + 2.0f * au, u1 + 3.0f * au};
	v4sf v_4 = {v1, v1 + av, v1 + 2.0f * av, v1 + 3.0f * av};
	v4sf w_4 = {w1, w1 + aw, w1 + 2.0f * aw, w1 + 3.0f * aw};

	v4sf au_4 = {au * 4.0f, au * 4.0f, au * 4.0f, au * 4.0f};
	v4sf av_4 = {av * 4.0f, av * 4.0f, av * 4.0f, av * 4.0f};
	v4sf aw_4 = {aw * 4.0f, aw * 4.0f, aw * 4.0f, aw * 4.0f};

	int xb = (int) floorf(x1);
	int xe = (int) ceilf(x2);
	uint32_t * p = xb + ((int) y) * frame.tx + (uint32_t *) frame.data;
	int b = (xe - xb) >> 2;
	int r = (xe - xb) & 0x3;

	for (int x = 0; x < b; x ++) {
		v4sf z_4;
		z_4.v = __builtin_ia32_rcpps(w_4.v);

		v4sf mu_4, mv_4;
		mu_4.v = u_4.v * z_4.v;
		mv_4.v = v_4.v * z_4.v * texScale_4.v;

		v4si mui_4, mvi_4;
		mui_4.v = (V4SI) _mm_cvtps_epi32(mu_4.v);
		mvi_4.v = (V4SI) _mm_cvtps_epi32(mv_4.v);
		mui_4.v = (V4SI) _mm_and_si128((__m128i) mui_4.v, (__m128i) texMaskU_4.v);
		mvi_4.v = (V4SI) _mm_and_si128((__m128i) mvi_4.v, (__m128i) texMaskV_4.v);
		mui_4.v += mvi_4.v;

		v4si zv = {0, 0, 0, 0};
		v4si tp, tq, t1, t2;
		tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[0]]);
		tq.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[1]]);
		t1.v = (V4SI) _mm_unpacklo_epi32((__m128i)tp.v, (__m128i)tq.v);
		t1.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)t1.v);
		t1.v = (V4SI) _mm_mulhi_epu16((__m128i)t1.v, (__m128i)color_1.v);

		tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[2]]);
		tq.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[3]]);
		t2.v = (V4SI) _mm_unpacklo_epi32((__m128i)tp.v, (__m128i)tq.v);
		t2.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)t2.v);
		t2.v = (V4SI) _mm_mulhi_epu16((__m128i)t2.v, (__m128i)color_1.v);

		tp.v = (V4SI) _mm_packus_epi16((__m128i)t1.v, (__m128i)t2.v);
		_mm_storeu_si128((__m128i *) p, (__m128i) tp.v);
		p += 4;

		w_4.v += aw_4.v;
		u_4.v += au_4.v;
		v_4.v += av_4.v;
	}

	if (r == 0) return;
	v4sf z_4;
	z_4.v = __builtin_ia32_rcpps(w_4.v);
	v4sf mu_4, mv_4;
	mu_4.v = u_4.v * z_4.v;
	mv_4.v = v_4.v * z_4.v * texScale_4.v;

	v4si mui_4, mvi_4;
	mui_4.v = (V4SI) _mm_cvtps_epi32(mu_4.v);
	mvi_4.v = (V4SI) _mm_cvtps_epi32(mv_4.v);
	mui_4.v = (V4SI) _mm_and_si128((__m128i) mui_4.v, (__m128i) texMaskU_4.v);
	mvi_4.v = (V4SI) _mm_and_si128((__m128i) mvi_4.v, (__m128i) texMaskV_4.v);
	mui_4.v += mvi_4.v;

	v4si zv = {0, 0, 0, 0};
	v4si tp;
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[0]]);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);
	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);

	if (r == 1) return;
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[1]]);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);
	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);

	if (r == 2) return;
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[2]]);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);
	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);
}

inline void LeRasterizer::fillFlatTexAlphaZC(float y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
	float d = x2 - x1;
	if (d == 0.0f) return;

	float id = 1.0f / d;
	float au = (u2 - u1) * id;
	float av = (v2 - v1) * id;
	float aw = (w2 - w1) * id;

	v4sf u_4 = {u1, u1 + au, u1 + 2.0f * au, u1 + 3.0f * au};
	v4sf v_4 = {v1, v1 + av, v1 + 2.0f * av, v1 + 3.0f * av};
	v4sf w_4 = {w1, w1 + aw, w1 + 2.0f * aw, w1 + 3.0f * aw};

	v4sf au_4 = {au * 4.0f, au * 4.0f, au * 4.0f, au * 4.0f};
	v4sf av_4 = {av * 4.0f, av * 4.0f, av * 4.0f, av * 4.0f};
	v4sf aw_4 = {aw * 4.0f, aw * 4.0f, aw * 4.0f, aw * 4.0f};

	int xb = (int) floorf(x1);
	int xe = (int) ceilf(x2);
	uint32_t * p = xb + ((int) y) * frame.tx + (uint32_t *) frame.data;
	int b = (xe - xb) >> 2;
	int r = (xe - xb) & 0x3;

	v4si sc = {0x01000100, 0x01000100, 0x01000100, 0x01000100};
	for (int x = 0; x < b; x ++) {
		v4sf z_4;
		z_4.v = __builtin_ia32_rcpps(w_4.v);

		v4sf mu_4, mv_4;
		mu_4.v = u_4.v * z_4.v;
		mv_4.v = v_4.v * z_4.v * texScale_4.v;

		v4si mui_4, mvi_4;
		mui_4.v = (V4SI) _mm_cvtps_epi32(mu_4.v);
		mvi_4.v = (V4SI) _mm_cvtps_epi32(mv_4.v);
		mui_4.v = (V4SI) _mm_and_si128((__m128i) mui_4.v, (__m128i) texMaskU_4.v);
		mvi_4.v = (V4SI) _mm_and_si128((__m128i) mvi_4.v, (__m128i) texMaskV_4.v);
		mui_4.v += mvi_4.v;

		v4si zv = {0, 0, 0, 0};
		v4si tp, tq, fp, t1, t2;
		v4si ap, apl, aph;

		fp.v = (V4SI) _mm_loadl_epi64((__m128i *) p);
		tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[0]]);
		tq.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[1]]);
		t1.v = (V4SI) _mm_unpacklo_epi32((__m128i)tp.v, (__m128i)tq.v);
		fp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)fp.v);
		t1.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)t1.v);

		apl.v = (V4SI) _mm_shufflelo_epi16((__m128i)t1.v, 0xFF);
		aph.v = (V4SI) _mm_shufflehi_epi16((__m128i)t1.v, 0xFF);
		ap.v = (V4SI) _mm_move_sd((__m128d)aph.v, (__m128d)apl.v);
		ap.v = (V4SI) _mm_srli_epi16((__m128i)ap.v, 8);
		ap.v = (V4SI) _mm_sub_epi16((__m128i)sc.v, (__m128i)ap.v);

		t1.v = (V4SI) _mm_mulhi_epu16((__m128i)t1.v, (__m128i)color_1.v);
		fp.v = (V4SI) _mm_mulhi_epu16((__m128i)fp.v, (__m128i)ap.v);
		t1.v = (V4SI) _mm_adds_epu16((__m128i)t1.v, (__m128i)fp.v);

		fp.v = (V4SI) _mm_loadl_epi64((__m128i *) (p+2));
		tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[2]]);
		tq.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[3]]);
		t2.v = (V4SI) _mm_unpacklo_epi32((__m128i)tp.v, (__m128i)tq.v);
		fp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)fp.v);
		t2.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)t2.v);

		apl.v = (V4SI) _mm_shufflelo_epi16((__m128i)t2.v, 0xFF);
		aph.v = (V4SI) _mm_shufflehi_epi16((__m128i)t2.v, 0xFF);
		ap.v = (V4SI) _mm_move_sd((__m128d)aph.v, (__m128d)apl.v);
		ap.v = (V4SI) _mm_srli_epi16((__m128i)ap.v, 8);
		ap.v = (V4SI) _mm_sub_epi16((__m128i)sc.v, (__m128i)ap.v);

		t2.v = (V4SI) _mm_mulhi_epu16((__m128i)t2.v, (__m128i)color_1.v);
		fp.v = (V4SI) _mm_mulhi_epu16((__m128i)fp.v, (__m128i)ap.v);
		t2.v = (V4SI) _mm_adds_epu16((__m128i)t2.v, (__m128i)fp.v);

		tp.v = (V4SI) _mm_packus_epi16((__m128i)t1.v, (__m128i)t2.v);
		_mm_storeu_si128((__m128i *) p, (__m128i) tp.v);
		p += 4;

		w_4.v += aw_4.v;
		u_4.v += au_4.v;
		v_4.v += av_4.v;
	}

	if (r == 0) return;

	v4sf z_4;
	z_4.v = __builtin_ia32_rcpps(w_4.v);
	v4sf mu_4, mv_4;
	mu_4.v = u_4.v * z_4.v;
	mv_4.v = v_4.v * z_4.v * texScale_4.v;

	v4si mui_4, mvi_4;
	mui_4.v = (V4SI) _mm_cvtps_epi32(mu_4.v);
	mvi_4.v = (V4SI) _mm_cvtps_epi32(mv_4.v);
	mui_4.v = (V4SI) _mm_and_si128((__m128i) mui_4.v, (__m128i) texMaskU_4.v);
	mvi_4.v = (V4SI) _mm_and_si128((__m128i) mvi_4.v, (__m128i) texMaskV_4.v);
	mui_4.v += mvi_4.v;

	v4si zv = {0, 0, 0, 0};
	v4si tp, fp;
	fp.v = (V4SI) _mm_loadl_epi64((__m128i *) p);
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[0]]);
	fp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)fp.v);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);

	v4si ap;
	ap.v = (V4SI) _mm_shufflelo_epi16((__m128i)tp.v, 0xFF);
	ap.v = (V4SI) _mm_srli_epi16((__m128i)ap.v, 8);
	ap.v = (V4SI) _mm_sub_epi16((__m128i)sc.v, (__m128i)ap.v);

	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	fp.v = (V4SI) _mm_mulhi_epu16((__m128i)fp.v, (__m128i)ap.v);
	tp.v = (V4SI) _mm_adds_epu16((__m128i)tp.v, (__m128i)fp.v);

	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);

	if (r == 1) return;

	fp.v = (V4SI) _mm_loadl_epi64((__m128i *) p);
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[1]]);
	fp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)fp.v);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);

	ap.v = (V4SI) _mm_shufflelo_epi16((__m128i)tp.v, 0xFF);
	ap.v = (V4SI) _mm_srli_epi16((__m128i)ap.v, 8);
	ap.v = (V4SI) _mm_sub_epi16((__m128i)sc.v, (__m128i)ap.v);

	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	fp.v = (V4SI) _mm_mulhi_epu16((__m128i)fp.v, (__m128i)ap.v);
	tp.v = (V4SI) _mm_adds_epu16((__m128i)tp.v, (__m128i)fp.v);

	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);

	if (r == 2) return;
	fp.v = (V4SI) _mm_loadl_epi64((__m128i *) p);
	tp.v = (V4SI) _mm_loadl_epi64((__m128i *) &texPixels[mui_4.i[2]]);
	fp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)fp.v);
	tp.v = (V4SI) _mm_unpacklo_epi8((__m128i)zv.v, (__m128i)tp.v);

	ap.v = (V4SI) _mm_shufflelo_epi16((__m128i)tp.v, 0xFF);
	ap.v = (V4SI) _mm_srli_epi16((__m128i)ap.v, 8);
	ap.v = (V4SI) _mm_sub_epi16((__m128i)sc.v, (__m128i)ap.v);

	tp.v = (V4SI) _mm_mulhi_epu16((__m128i)tp.v, (__m128i)color_1.v);
	fp.v = (V4SI) _mm_mulhi_epu16((__m128i)fp.v, (__m128i)ap.v);
	tp.v = (V4SI) _mm_adds_epu16((__m128i)tp.v, (__m128i)fp.v);

	tp.v = (V4SI) _mm_packus_epi16((__m128i)tp.v, (__m128i)zv.v);
	*p++ = _mm_cvtsi128_si32((__m128i)tp.v);
}

#else

inline void LeRasterizer::fillFlatTexZC(float y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
	uint8_t * c = (uint8_t *) &color;

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	int xb = (int) floorf(x1);
	int xe = (int) ceilf(x2);
	uint8_t * p = (uint8_t *) (xb + ((int) y) * frame.tx + (uint32_t *) frame.data);

	for (int x = xb; x <= xe; x++) {
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t) (u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t) (v1 * z)) & texMaskV;
		uint8_t * t = (uint8_t *) &texPixels[tu + (tv << texSizeU)];

		p[0] = (t[0] * c[0]) >> 8;
		p[1] = (t[1] * c[1]) >> 8;
		p[2] = (t[2] * c[2]) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
}

inline void LeRasterizer::fillFlatTexAlphaZC(float y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
	uint8_t * c = (uint8_t *) &color;

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	int xb = (int) floorf(x1);
	int xe = (int) ceilf(x2);
	uint8_t * p = (uint8_t *) (xb + ((int) y) * frame.tx + (uint32_t *) frame.data);

	for (int x = xb; x <= xe; x++) {
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t) (u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t) (v1 * z)) & texMaskV;
		uint8_t * t = (uint8_t *) &texPixels[tu + (tv << texSizeU)];

		uint8_t a = t[3] ^ 0xFF;
		p[0] = (p[0] * a + t[0] * c[0]) >> 8;
		p[1] = (p[1] * a + t[1] * c[1]) >> 8;
		p[2] = (p[2] * a + t[2] * c[2]) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
}

#endif // LE_USE_SIMD && LE_USE_SSE2

#endif // LE_RENDERER_INTRASTER == 0
