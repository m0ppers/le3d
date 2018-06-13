/**
	\file timing_amiga.cpp
	\brief LightEngine 3D (tools): Native OS time measurement
	\brief Amiga OS implementation
	\author Andreas Streichardt (andreas@mop.koeln)
	\twitter @m0ppers
	\website https://mop.koeln
	\copyright Frederic Meslin 2015 - 2018
	\version 1.7

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

/*****************************************************************************/
#if defined(AMIGA)

#include "config.h"

#include "timing.h"

#include <stdio.h>

#if LE_USE_VBLANK_FRAME_WAIT == 1
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/types.h>
#endif
#include <hardware/intbits.h>
#include <proto/exec.h>
#include <proto/timer.h>

/*****************************************************************************/
LeTiming timing;

struct timerequest* timereq = NULL;
struct MsgPort *port = NULL;
struct EClockVal ecval;
struct Device* TimerBase = NULL;

#if LE_USE_VBLANK_FRAME_WAIT == 1
struct Interrupt *vbint;
static volatile ULONG frameCounter = 0;

// LOL different ABI?!??! a1 has the dataptr (wtf?)
// luckily we can access the static variable :S
void VertBServer() {
	frameCounter++;
}
#endif

/*****************************************************************************/
LeTiming::LeTiming() :
	fps(0.0f),
    enableDisplay(true),
	countsPerSec(0), countsPerFrame(0),
	lastCounter(0)
{
#if LE_USE_VBLANK_FRAME_WAIT == 1
	vbint = (struct Interrupt*) AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR);
	if (!vbint) {
		printf("Couldn't alloc interrupt");
		return;
	}

	vbint->is_Node.ln_Type = NT_INTERRUPT;         /* Initialize the node. */
	vbint->is_Node.ln_Pri = -60;
    vbint->is_Node.ln_Name = (char*) "le3d";
    vbint->is_Data = (APTR)&frameCounter;
    vbint->is_Code = VertBServer;

	AddIntServer(INTB_VERTB, vbint); /* Kick this interrupt server to life. */
#endif
	port = CreateMsgPort();
	if (!port) {
		printf("Couldn't create message port!\n");
		return;
	}
	timereq = (timerequest*) AllocMem(sizeof(timerequest), MEMF_CLEAR|MEMF_PUBLIC);
	if (!timereq) {
		printf("Couldn't alloc timerequest!\n");
		return;
	}

	uint32_t error = OpenDevice("timer.device", UNIT_WAITECLOCK, &timereq->tr_node, 0);
	if (error > 0) {
		printf("Couldn't open timer.device %d!\n", error);
		return;
	}
	TimerBase = (struct Device*) timereq->tr_node.io_Device;
	timereq->tr_node.io_Message.mn_ReplyPort = port;
	timereq->tr_node.io_Command = TR_ADDREQUEST;
}

LeTiming::~LeTiming()
{
	if (timereq) {
		CloseDevice(&timereq->tr_node);
		FreeMem(timereq, sizeof(timerequest));
	}
	if (port) {
		DeleteMsgPort(port);
	}
#if LE_USE_VBLANK_FRAME_WAIT == 1
	RemIntServer(INTB_VERTB, vbint);
	FreeMem(vbint, sizeof(struct Interrupt));
#endif
}

/*****************************************************************************/
/**
	\fn void LeTiming::setup(int targetFPS)
	\brief Configure the frame timing system
	\param[in] targetFPS desired application FPS
*/
void LeTiming::setup(int targetFPS)
{
	countsPerSec = ReadEClock(&ecval);
	
	countsPerFrame = countsPerSec / targetFPS;
	
	printf("Timing: counts per seconds: %d\n", (int) countsPerSec);
	printf("Timing: counts per frame: %d\n", (int) countsPerFrame);
}

/*****************************************************************************/
/**
	\fn void LeTiming::firstFrame()
	\brief Mark the time of the first frame
*/
void LeTiming::firstFrame()
{
	ReadEClock(&ecval);
	lastCounter = ((int64_t) ecval.ev_hi << 32) | ecval.ev_lo;
}

/**
	\fn void LeTiming::lastFrame()
	\brief Mark the time of the last frame
*/
void LeTiming::lastFrame()
{
}

/*****************************************************************************/
/**
	\fn bool LeTiming::isNextFrame()
	\brief Is it the time to display the next frame?
	\return true if it is time, false else
*/
bool LeTiming::isNextFrame()
{
	ReadEClock(&ecval);

	int64_t pc = ((int64_t) ecval.ev_hi << 32) | ecval.ev_lo;
	int64_t dt = pc - lastCounter;
	if (dt < countsPerFrame) return false;
	lastCounter = pc;

    fps = (float) countsPerSec / dt;
    if (enableDisplay) display();
	return true;
}

/**
	\fn void LeTiming::waitNextFrame()
	\brief Wait until it is time to display the next frame
*/
void LeTiming::waitNextFrame()
{
#if LE_USE_VBLANK_FRAME_WAIT == 1
	// framecounter will be incremented whenever vblank interrupt was triggered
	// so as long as this is 0 the current frame did not finish and we must wait
	while (frameCounter == 0);
	frameCounter = 0;
#else
	while (1) {
		ReadEClock(&ecval);

		int64_t pc = ((int64_t) ecval.ev_hi << 32) | ecval.ev_lo;

		int64_t dt = pc - lastCounter;
		int64_t dg = countsPerFrame - dt;
		if (dg <= 0) {
			break;
		}

		ULONG us = dg / 1000000;
		if (us > 0) {
			SendIO(&timereq->tr_node);
			WaitPort(port);
			GetMsg(port);
		}
	}
#endif
	ReadEClock(&ecval);
	int64_t pc = ((int64_t) ecval.ev_hi << 32) | ecval.ev_lo;
	int64_t dt = pc - lastCounter;
	lastCounter = pc;

	fps = (float) countsPerSec / dt;
	if (enableDisplay) display();

}

/*****************************************************************************/
/**
	\fn void LeTiming::display()
	\brief Display the current application FPS in the console
*/
void LeTiming::display()
{
	static int ttd = 0;
	if (ttd++ == 30) {
		printf("FPS %f\n", fps);
		ttd = 0;
	}
}

#endif
