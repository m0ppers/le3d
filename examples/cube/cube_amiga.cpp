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
const int resoX = 480;
const int resoY = 270;

/*****************************************************************************/
int main()
{
/** Load the assets (textures then 3D models) */
	bmpCache.loadDirectory("assets");
	meshCache.loadDirectory("assets");
	
/** Create application objects */
	LeWindow	 window		= LeWindow("Le3d: amiga", resoX, resoY);
	LeDraw		 draw		= LeDraw(window.getContext(), resoX, resoY);
	LeRenderer	 renderer	= LeRenderer(resoX, resoY);
	LeRasterizer rasterizer = LeRasterizer(resoX, resoY);

    // std::cout << "Setup done!" << std::endl;


// /** Retrieve the 3D model */
	int crateSlot = meshCache.getFromName("crate.obj");
	LeMesh * crate = meshCache.cacheSlots[crateSlot].mesh;

// /** Create three lights */
	LeLight light1(LE_LIGHT_DIRECTIONAL, LeColor::rgb(0xFF4040));
	LeLight light2(LE_LIGHT_DIRECTIONAL, LeColor::rgb(0x4040FF));
	LeLight light3(LE_LIGHT_AMBIENT, LeColor::rgb(0x404040));

	light1.axis = LeAxis(LeVertex(), LeVertex(1.0f, 0.0f, -1.0f));
	light2.axis = LeAxis(LeVertex(), LeVertex(-1.0f, 0.0f, -1.0f));

/** Set the renderer properties */
	renderer.setViewPosition(LeVertex(0.0f, 0.0f, 3.0f));
	renderer.updateViewMatrix();
	rasterizer.background = LeColor::rgb(0xC0C0FF);

/** Initialize the timing */
	timing.setup(60);
	timing.firstFrame();
	
	LeBitmap off1;
	off1.allocate(resoX, resoY + 2);
	LeBitmap off2;
	off2.allocate(resoX, resoY + 2);
	
	LeBitmap* fb1 = &rasterizer.frame;
	LeBitmap* fb2 = &off1;
	LeBitmap* fb3 = &off2;
/** Program main loop */
	int frameCounter = 0;
	while (window.update()) {
		// XEvent event;
	//	XNextEvent(usedXDisplay, &event);

		// if (event.type == KeyPress)
		// 	break;

	/** Wait for next frame */
		timing.waitNextFrame();
	// /** Copy render frame to window context */
		draw.setPixels(fb1->data);
		void* tmp = fb1->data;
		fb1->data = fb2->data;
		fb2->data = fb3->data;
		fb3->data = tmp;

	/** Update model transforms */
		crate->angle += LeVertex(0.1f, 2.0f, 0.0f);
		crate->updateMatrix();

	/** Light model */
		LeLight::black(crate);
		light1.shine(crate);
		light2.shine(crate);
		light3.shine(crate);

	/** Render the 3D model */
		renderer.render(crate);
	/** Draw the triangles */
		rasterizer.flush();
		rasterizer.rasterList(renderer.getTriangleList());
		renderer.flush();
	}

	timing.lastFrame();

	return 0;
}
