/**
	\file cube.cpp
	\brief LightEngine 3D (examples): cube example
	\brief Unix OS implementation
	\author Frederic Meslin (fred@fredslab.net)
	\twitter @marzacdev
	\website http://fredslab.net
*/

#include "engine/le3d.h"
#include "tools/timing.h"

#include <proto/timer.h>
#include <stdlib.h>
#include <stdio.h>

/*****************************************************************************/
const int resoX = 640;
const int resoY = 360;

/*****************************************************************************/
int main()
{
/** Create application objects */
	LeWindow	 window		= LeWindow("Le3d: amiga", resoX, resoY);
	LeDrawingContext dc     = window.getContext();
	LeDraw		 draw		= LeDraw(dc, resoX, resoY);
	LeRenderer	 renderer	= LeRenderer(resoX, resoY);
	LeRasterizer rasterizer = LeRasterizer(resoX, resoY);

    // std::cout << "Setup done!" << std::endl;

/** Load the assets (textures then 3D models) */
	bmpCache.loadDirectory("assets");
	meshCache.loadDirectory("assets");

// /** Retrieve the 3D model */
	int crateSlot = meshCache.getFromName("crate.obj");
	LeMesh * crate = meshCache.slots[crateSlot].mesh;

// /** Create three lights */
	LeLight light1(LE_LIGHT_DIRECTIONAL, 0xFF4040);
	LeLight light2(LE_LIGHT_DIRECTIONAL, 0x4040FF);
	LeLight light3(LE_LIGHT_AMBIENT, 0x404040);

	light1.axis = LeAxis(LeVertex(), LeVertex(1.0f, 0.0f, -1.0f));
	light2.axis = LeAxis(LeVertex(), LeVertex(-1.0f, 0.0f, -1.0f));

/** Set the renderer properties */
	renderer.setViewPosition(LeVertex(0.0f, 0.0f, 3.0f));
	renderer.updateViewMatrix();
	rasterizer.background = 0xC0C0FF;

/** Initialize the timing */
	timing.setup(60);
	timing.firstFrame();

/** Program main loop */
	int frameCounter = 0;
	while (window.update()) {
		struct EClockVal start;
		struct EClockVal current;
		bool test = frameCounter == 10;

		// XEvent event;
	//	XNextEvent(usedXDisplay, &event);

		// if (event.type == KeyPress)
		// 	break;

	/** Wait for next frame */
		if (test) {
			ReadEClock(&start);
		}
		timing.waitNextFrame();
		if (test) {
			ReadEClock(&current);
			printf("timing.waitNextFrame %u\n", current.ev_lo - start.ev_lo);
		}

	// /** Copy render frame to window context */
		if (!test) {
			ReadEClock(&start);
		}

		draw.setPixels(rasterizer.frame.data);
		if (test) {
			ReadEClock(&current);
			printf("draw.setPixels %u\n", current.ev_lo - start.ev_lo);
		}

	/** Update model transforms */
		if (test) {
			ReadEClock(&start);
		}

		crate->angle += LeVertex(0.1f, 2.0f, 0.0f);
		crate->updateMatrix();
		if (test) {
			ReadEClock(&current);
			printf("crate->updateMatrix %u\n", current.ev_lo - start.ev_lo);
		}


	/** Light model */
		if (test) {
			ReadEClock(&start);
		}

		LeLight::black(crate);
		if (test) {
			ReadEClock(&current);
			printf("LeLight::black %u\n", current.ev_lo - start.ev_lo);
		}
		if (test) {
			ReadEClock(&start);
		}

		light1.shine(crate);
		if (test) {
			ReadEClock(&current);
			printf("light1.shine %u\n", current.ev_lo - start.ev_lo);
		}
		if (test) {
			ReadEClock(&start);
		}

		light2.shine(crate);
		if (test) {
			ReadEClock(&current);
			printf("light2.shine %u\n", current.ev_lo - start.ev_lo);
		}
		if (test) {
			ReadEClock(&start);
		}

		light3.shine(crate);
		if (test) {
			ReadEClock(&current);
			printf("light3.shine %u\n", current.ev_lo - start.ev_lo);
		}

	/** Render the 3D model */
		if (test) {
			ReadEClock(&start);
		}

		renderer.render(crate);
		if (test) {
			ReadEClock(&current);
			printf("renderer.render %u\n", current.ev_lo - start.ev_lo);
		}

	/** Draw the triangles */
		if (test) {
			ReadEClock(&start);
		}

		rasterizer.flush();
		if (test) {
			ReadEClock(&current);
			printf("rasterizer.flush() %u\n", current.ev_lo - start.ev_lo);
		}
		if (test) {
			ReadEClock(&start);
		}

		rasterizer.rasterList(renderer.getTriangleList());
		if (test) {
			ReadEClock(&current);
			printf("rasterizer.rasterList %u\n", current.ev_lo - start.ev_lo);
		}
		if (test) {
			ReadEClock(&start);
		}

		renderer.flush();
		if (test) {
			ReadEClock(&current);
			printf("renderer.flush() %u\n", current.ev_lo - start.ev_lo);
		}

		frameCounter++;
	}

	timing.lastFrame();

	return 0;
}
