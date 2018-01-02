/**
	\file timing.cpp
	\brief LightEngine 3D (tools): Native OS time measurement
	\brief Windows OS implementation
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


#ifndef LE_TIMING_H
#define LE_TIMING_H

#include <stdint.h>
#include <sys/time.h>

/*****************************************************************************/
#define LE_TIMING_FPS			30		/*< Target FPS */
#define LE_TIMING_GRANULARITY	1       /*< Desired scheduler granularity (in ms) */

/*****************************************************************************/
class LeTiming {
public:
	LeTiming();
	~LeTiming();

	void firstFrame();
	void lastFrame();

	bool isNextFrame();
    void waitNextFrame();

public:
	float fps;				    /*< Current game FPS */
	float targetFps;		    /*< Target game FPS */
    bool enableFPSDisplay;      /*< Periodically displays current FPS in console */

private:
	int64_t countsPerSec;
	int64_t countsPerFrame;
	int64_t lastCounter;

    void displayFPS();
};

#endif // LE_TIMING_H