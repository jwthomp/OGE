// OGE.cpp : Defines the entry point for the application.
//


//#include <gl/gl.h>
//#include <gl/glu.h>

#ifdef IO_SCRIPTING
#include "Headers/IoState.h"
#endif

#include "core_lib.h"
#include "render_lib.h"
#include "physics_lib.h"
#include "input_lib.h"
#include "frametime.h"
#include "resource_manager.h"		
#include "particle_system.h"
#include "mesh.h"
#include "cape.h"
#include "obj_cloth.h"
#include "mesh_generator.h"
#include "objects_guff.h"
#include "quaternion.h"
#include "matrix.h"
#include "assert.h"
#include "scene_manager.h"
#include "sg_main_scene.h"
#include "shader.h"

#include "SDL.h"
#include "SDL_OpenGL.h"


#include <stdio.h>
#include <math.h>

#include <list>

#include <windows.h>

sg_main_scene g_main_scene;

static void quit_tutorial( int code )
{
	/*
	 * Quit SDL so we can release the fullscreen
	 * mode and restore the previous video settings,
	 * etc.
	 */
	SDL_Quit( );
	
	/* Exit program. */
	exit( code );
}



int main(int argc, char *argv[]) {
	
	/* Dimensions of our window. */
	int width = 1280;
	int height = 720;
	
   
	/*
	 * At this point, we should have a properly setup
	 * double-buffered window for use with OpenGL.
	 */
	if (core_lib_init() == false) {
		return 1;
	}
	
	render_lib_init(width, height);
	
	if (input_lib_init() == false) {
		return 1;
	}
	
	frametime_init();

	render_lib_set_default_shader("deferred_base");
	
	
	//g_obj.init();
	//g_obj.load_obj("head.obj");
	//g_obj.load_obj("OGE-osx.app/Contents/Resources/head.obj");
	//g_obj.load_obj("ship.obj");
	//physics_lib_cloth_sim_add(&g_obj);
	
	objects_guff_demi_redeems();
	

	
#if 0
	objects_guff_create_instance(g_mesh_Ground);
	objects_guff_create_instance(g_mesh_CityBlockA);
	objects_guff_create_instance(g_mesh_CityBlockB);
	objects_guff_create_instance(g_mesh_CityBlockC);
	objects_guff_create_instance(g_mesh_CityBlockD);
	objects_guff_create_instance(g_mesh_CityBlockE);
	objects_guff_create_instance(g_mesh_CityBlockF);
	objects_guff_create_instance(g_mesh_CityBlockG);
	objects_guff_create_instance(g_mesh_CityBlockH);
	objects_guff_create_instance(g_mesh_CityBlockI);
	objects_guff_create_instance(g_mesh_CityBlockJ);
	objects_guff_create_instance(g_mesh_CityBlockK);
	objects_guff_create_instance(g_mesh_CityBlockL);
	objects_guff_create_instance(g_mesh_CityBlockM);
	objects_guff_create_instance(g_mesh_CityBlockN);
	objects_guff_create_instance(g_mesh_CityBlockO);
	objects_guff_create_instance(g_mesh_CityBlockP);
	objects_guff_create_instance(g_mesh_CityBlockQ);
	
	
	objects_guff_create_instance(g_mesh_Ground);
	
	
	quaternion qt;
	
	mesh_instance *mi;
	mi = objects_guff_create_instance(g_mesh_CompoundBuildingD, 3400.0f, 1000.0f, 5800.0f);
	qt.CreateFromAxisAngle(0.0f, 1.0f, 0.0f, 180.0f);
	mi->m_transform.set_values(NULL, &qt, NULL);
	
	mi = objects_guff_create_instance(g_mesh_CompoundBuildingD, 5800.0f, 1000.0f, 3600.0f);
	qt.CreateFromAxisAngle(0.0f, 1.0f, 0.0f, -90.0f);
	mi->m_transform.set_values(NULL, &qt, NULL);
	
	
	mi = objects_guff_create_instance(g_mesh_BuildingB, -5000.0f, 1000.0f, 8600.0f);
	Vector3 scale;
	scale.set(2.0f, 10.0f, 2.0f);
	mi->m_transform.set_values(NULL, &qt, &scale);
	
	mi = objects_guff_create_instance(g_mesh_BuildingB, -5000.0f, 1000.0f, -8600.0f);
	mi->m_transform.set_values(NULL, &qt, &scale);
#endif		
	
	
	
#ifdef IO_SCRIPTING
	{
		IoState *self = IoState_new();
		IoState_init(self);
		IoState_doCString_(self, "writeln(\"hello world!\");");
		IoState_free(self);
	}
#endif
	
	g_main_scene.init();
	
	scene_manager_set_current(&g_main_scene);

	/*
	 * Now we want to begin our normal app process--
	 * an event loop with a lot of redrawing.
	 */
	while( 1 ) {
		real frametime = frametime_get() / 1000.0f;
		
		
		{
			static int count = 0;
			static int compare = 100;
			if (frametime > 0.005f) {
				compare = (int)(1.0f / frametime);
			}
			if (count >= compare) {
				char buffer[256];
				sprintf(buffer, "%u] ft: %f\n", frametime_get_count(), frametime);
				OutputDebugStringA(buffer);
				count = 0;
			}
			count++;
		}

		/* Process incoming events. */
		//process_events( );
		
		input_lib_process(frametime);
		
		g_main_scene.process(frametime);
		
		// Simulate away
		physics_lib_simulate(frametime);
		
		// Render away
		//render_lib_set_camera(g_camera_pos, g_camera_orient);
		render_lib_render();
		
		frametime_process();
	}
	
	/* Never reached. */
	return 0;
	
}
