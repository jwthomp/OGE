#include "input_lib.h"

#include "core_types.h"
#include "assert.h"
#include "input_event.h"
#include "scene_manager.h"
#include "scene_layer.h"
#include "scene_base.h"

#include "SDL.h"

// Process_events: First pushes events to menus if a related event isn't in the works for the scene (already pushed)
//							Then asks menus if can pass event to scene
//							If not, sets rest state for scene
static void process_events(real p_frametime)
{
	scene_base *current_scene = scene_manager_get_current();
	assert(current_scene);
	current_scene->clear_input_state();

	input_event_process(current_scene, p_frametime);		
}




bool input_lib_init()
{
	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0 ) {
		/* Failed, exit. */
		fprintf( stderr, "Input initialization failed: %s\n",
					SDL_GetError( ) );
		SDL_Quit( );
		return false;
	}
	
	unsigned long num_joysticks = SDL_NumJoysticks();
	for (unsigned long i = 0; i < num_joysticks; i++) {
		// Open joystick
		SDL_JoystickOpen(i);
	}
	
	return true;
}

bool input_lib_shutdown()
{
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	return true;
}

void input_lib_process(real p_frametime)
{
	process_events(p_frametime);
}


uint8 input_lib_get_joystick_count()
{
	uint32 num_joysticks = SDL_NumJoysticks();
	assert(num_joysticks < 256);
	return (uint8)num_joysticks;
}


//input_state const *input_lib_get_input_state_for_frame()
//{
//	return &g_input_state_for_frame;
//}