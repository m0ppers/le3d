/**
	\file bmpfile.cpp
	\brief LightEngine 3D: Windows bitmap file importer / exporter
	\brief All platforms implementation
	\author Frederic Meslin (fred@fredslab.net)
	\twitter @marzacdev
	\website http://fredslab.net
	\copyright Frederic Meslin 2015 - 2017
	\version 1.0

	The MIT License (MIT)
	Copyright (c) 2017 Fr�d�ric Meslin

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

#include "bmpfile.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*****************************************************************************/
#pragma pack(push, 1)
typedef struct {
	uint16_t bfType;
	uint32_t bfSize;
	int16_t  bfReserved1;
	int16_t  bfReserved2;
	uint32_t bfOffBits;
} BMPFILEHEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	int16_t  biPlanes;
	int16_t  biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BMPINFOHEADER;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  uint32_t CSType;
  uint32_t Endpoints[9];
  uint32_t GammaRed;
  uint32_t GammaGreen;
  uint32_t GammaBlue;
} BMPCOLORSPACE;

#pragma pack(push, 1)
typedef struct {
	uint32_t mR;
	uint32_t mG;
	uint32_t mB;
	uint32_t mA;
} BMPCOLORMASK;
#pragma pack(pop)

/*****************************************************************************/
#define BI_RGB				0
#define BI_BITFIELDS		3
#define BI_ALPHABITFIELDS	6

/*****************************************************************************/
LeBmpFile::LeBmpFile(const char * filename) :
	path(NULL)
{
	if (filename) path = strdup(filename);
}

LeBmpFile::~LeBmpFile()
{
	if (path) free(path);
}

/*****************************************************************************/
LeBitmap * LeBmpFile::load()
{
	FILE * file = fopen(path, "rb");
	if (!file) {
		printf("bmpFile: File not found %s!\n", path);
		return NULL;
	}
	LeBitmap * bitmap = new LeBitmap();
	if (!bitmap) {
		fclose(file);
		return NULL;
	}
	readBitmap(file, bitmap);
	fclose(file);
	return bitmap;
}


void LeBmpFile::save(LeBitmap * bitmap)
{
	FILE * file = fopen(path, "wb");
	if (!file) {
		printf("bmpFile: File cannot be opened %s!\n", path);
		return;
	}
	writeBitmap(file, bitmap);
	fclose(file);
}

/*****************************************************************************/
int LeBmpFile::readBitmap(FILE * file, LeBitmap * bitmap)
{
	BMPFILEHEADER header;
	BMPINFOHEADER info;
	BMPCOLORMASK  mask = {
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000
	};
// Read the headers
	fread(&header, sizeof(BMPFILEHEADER), 1, file);
	fread(&info, sizeof(BMPINFOHEADER), 1, file);

// Check bitmap format
	if (strncmp((char *) &header.bfType, "BM", 2)){
		printf("bmpFile: File not a bitmap!\n");
		return 0;
	}
	if (info.biBitCount != 24 && info.biBitCount != 32) {
		printf("bmpFile: Only 24bit or 32bit bitmaps are supported!\n");
		return 0;
	}
	if (info.biCompression != BI_RGB && info.biCompression != BI_BITFIELDS){
		printf("bmpFile: Only uncompressed formats are supported!\n");
		return 0;
	}

// Load the bitmasks
	int shiftA = 24;
	int shiftR = 16;
	int shiftG = 8;
	int shiftB = 0;
	if (info.biCompression == BI_BITFIELDS) {
		fread(&mask, sizeof(BMPCOLORMASK), 1, file);
		shiftR = __builtin_ffs(mask.mR) - 1;
		shiftG = __builtin_ffs(mask.mG) - 1;
		shiftB = __builtin_ffs(mask.mB) - 1;
		shiftA = __builtin_ffs(mask.mA) - 1;
	}

// Retrieve bitmap size
	bitmap->tx = info.biWidth;
	bitmap->ty = info.biHeight;
	bitmap->txP2 = (int) log2f((float)bitmap->tx);
	bitmap->tyP2 = (int) log2f((float)bitmap->ty);

	int upsidedown = 1;
	if (bitmap->ty < 0) {
		bitmap->ty = -bitmap->ty;
		upsidedown = 0;
	}

// Set bitmap flags
	bitmap->flags = LE_BMP_DEFAULT;
	if (info.biBitCount == 32)
		bitmap->flags |= LE_BMP_ALPHACHANNEL;

// Allocate bitmap memory
	int srcScan;
	srcScan = bitmap->tx * (info.biBitCount >> 3);
	srcScan = (srcScan + 0x3) & ~0x3;
	int size = bitmap->tx * bitmap->ty * sizeof(uint32_t);
	bitmap->data = malloc(size);
	bitmap->allocated = true;
	uint8_t * buffer = (uint8_t *) malloc(srcScan);
	uint8_t * data = (uint8_t *) bitmap->data;

// Load bitmap data
	int dstScan = bitmap->tx * sizeof(uint32_t);
	fseek(file, header.bfOffBits, SEEK_SET);
	if (upsidedown)
		data += dstScan * (bitmap->ty - 1);

	if (info.biBitCount == 32) {
	// Parse a 32 bits image
		for (int y = 0; y < bitmap->ty; y ++) {
			fread(buffer, srcScan, 1, file);
			uint32_t * d = (uint32_t *) data;
			uint32_t * s = (uint32_t *) buffer;
			for (int i = 0; i < bitmap->tx; i ++) {
				uint32_t c = *s++;
				uint32_t a, r, g, b;
				a = (c & mask.mA) >> shiftA;
				r = (c & mask.mR) >> shiftR;
				g = (c & mask.mG) >> shiftG;
				b = (c & mask.mB) >> shiftB;
				*d++ = (a << 24) | (r << 16) | (g << 8) | b;
			}
			if (upsidedown)
				data -= dstScan;
			else data += dstScan;
		}
	}else{
	// Parse a 24 bits image
		for (int y = 0; y < bitmap->ty; y ++) {
			fread(buffer, srcScan, 1, file);
			uint32_t * d = (uint32_t *) data;
			uint8_t  * s = buffer;
			for (int i = 0; i < bitmap->tx; i ++) {
				uint32_t r, g, b;
				b = * s++;
				g = * s++;
				r = * s++;
				* d++ = (r << 16) | (g << 8) | b;
			}
			if (upsidedown)
				data -= dstScan;
			else data += dstScan;
		}
	}
	free(buffer);
	return 1;
}


/*****************************************************************************/
#define HEAD_LEN sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER) + sizeof(BMPCOLORMASK)
int LeBmpFile::writeBitmap(FILE * file, LeBitmap * bitmap)
{
// Prepare the headers
	size_t size = bitmap->tx * bitmap->ty * sizeof(uint32_t);
	BMPFILEHEADER header;
	header.bfType = 0x4D42;
	header.bfSize = HEAD_LEN + size;
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = HEAD_LEN;

	BMPINFOHEADER info;
	info.biSize = sizeof(BMPINFOHEADER) + sizeof(BMPCOLORMASK);
	info.biWidth = bitmap->tx;
	info.biHeight = bitmap->ty;
	info.biPlanes = 1;
	info.biBitCount = 32;
	info.biCompression = BI_BITFIELDS;
	info.biSizeImage = 0;
	info.biXPelsPerMeter = 96;
	info.biYPelsPerMeter = 96;
	info.biClrUsed = 0;
	info.biClrImportant = 0;

	BMPCOLORMASK mask = {
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000
	};

// Write the headers
	fwrite(&header, sizeof(BMPFILEHEADER), 1, file);
	fwrite(&info, sizeof(BMPINFOHEADER), 1, file);
	fwrite(&mask, sizeof(BMPCOLORMASK), 1, file);

// Save the picture
	uint32_t scan = bitmap->tx * sizeof(uint32_t);
	uint8_t * data = (uint8_t *) bitmap->data;
	for (int y = 0; y < bitmap->ty; y ++) {
		fwrite(data, scan, 1, file);
		data += scan;
	}
	return 0;
}