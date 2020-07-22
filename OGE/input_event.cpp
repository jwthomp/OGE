#include "input_event.h"

#include "scene_base.h"

#include "SDL.h"

#pragma message(__FILE__ "    TODO: FULLY Implement event_sdl_to_oge")

static bool convert_sdl_keydown_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_KEYDOWN;

	bool event_processed = true;

	switch( p_sdl_event->key.keysym.sym ) {
		case SDLK_ESCAPE:
			p_event->m_event_keyboard.m_key = KEY_ESCAPE;
			break;
		case SDLK_SPACE:
			p_event->m_event_keyboard.m_key = KEY_SPACE;		
			break;
		case SDLK_w:
			p_event->m_event_keyboard.m_key = KEY_W;
			break;
		case SDLK_s:
			p_event->m_event_keyboard.m_key = KEY_S;
			break;
		case SDLK_a:
			p_event->m_event_keyboard.m_key = KEY_A;		
			break;
		case SDLK_d:
			p_event->m_event_keyboard.m_key = KEY_D;
			break;
		default:
			event_processed = false;
			break;
	}

	return event_processed;
}

static bool convert_sdl_keyup_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_KEYUP;

	bool event_processed = true;

	switch( p_sdl_event->key.keysym.sym ) {
		case SDLK_ESCAPE:
			p_event->m_event_keyboard.m_key = KEY_ESCAPE;
			break;
		case SDLK_SPACE:
			p_event->m_event_keyboard.m_key = KEY_SPACE;		
			break;
		case SDLK_w:
			p_event->m_event_keyboard.m_key = KEY_W;
			break;
		case SDLK_s:
			p_event->m_event_keyboard.m_key = KEY_S;
			break;
		case SDLK_a:
			p_event->m_event_keyboard.m_key = KEY_A;		
			break;
		case SDLK_d:
			p_event->m_event_keyboard.m_key = KEY_D;
			break;
		default:
			event_processed = false;
			break;
	}

	return event_processed;
}

static bool convert_sdl_joyaxismotion_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_JOYAXISMOTION;
	p_event->m_event_joystick_motion.m_joystick_index = p_sdl_event->jaxis.which;
	p_event->m_event_joystick_motion.m_axis_index = p_sdl_event->jaxis.axis;

	if (abs(p_sdl_event->jaxis.value) < 10000) {
		p_event->m_event_joystick_motion.m_axis_value = 0;
	} else {
		p_event->m_event_joystick_motion.m_axis_value = p_sdl_event->jaxis.value;
	}

	return true;
}

static bool convert_sdl_joybuttondown_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_JOYBUTTONDOWN;
	p_event->m_event_joystick_button_down.m_joystick_index = p_sdl_event->jbutton.which;
	p_event->m_event_joystick_button_down.m_button_index = p_sdl_event->jbutton.button;
	return true;
}

static bool convert_sdl_joybuttonup_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_JOYBUTTONUP;
	p_event->m_event_joystick_button_down.m_joystick_index = p_sdl_event->jbutton.which;
	p_event->m_event_joystick_button_down.m_button_index = p_sdl_event->jbutton.button;
	return true;
}


static bool convert_sdl_mousemotion_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_MOUSEMOTION;

	p_event->m_event_mouse_motion.m_axis_x = p_sdl_event->motion.x;
	p_event->m_event_mouse_motion.m_axis_y = p_sdl_event->motion.y;
	p_event->m_event_mouse_motion.m_axis_relx = p_sdl_event->motion.xrel;
	p_event->m_event_mouse_motion.m_axis_rely = p_sdl_event->motion.yrel;
	return true;
}

static bool convert_sdl_mousebuttonup_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_MOUSEBUTTONUP;
	p_event->m_event_mouse_button_up.m_button = p_sdl_event->button.button;
	return true;
}

static bool convert_sdl_mousebuttondown_event(SDL_Event const * p_sdl_event, input_event * p_event)
{
	p_event->m_event_type = INPUT_EVENT_TYPE_MOUSEBUTTONDOWN;
	p_event->m_event_mouse_button_up.m_button = p_sdl_event->button.button;
	return true;
}

static bool event_sdl_to_oge(SDL_Event const* p_sdl_event, input_event *p_event)
{
	switch(p_sdl_event->type ) {
		case SDL_KEYDOWN:
			return convert_sdl_keydown_event(p_sdl_event, p_event);
			break;
		case SDL_KEYUP:
			return convert_sdl_keyup_event(p_sdl_event, p_event);
			break;
		case SDL_JOYAXISMOTION:
		{
			return convert_sdl_joyaxismotion_event(p_sdl_event, p_event);
			break;
		}
		case SDL_JOYBUTTONDOWN:
			return convert_sdl_joybuttondown_event(p_sdl_event, p_event);

			break;
		case SDL_JOYBUTTONUP:
			return convert_sdl_joybuttonup_event(p_sdl_event, p_event);
			break;
		case SDL_MOUSEMOTION:
		{
			return convert_sdl_mousemotion_event(p_sdl_event, p_event);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			return convert_sdl_mousebuttondown_event(p_sdl_event, p_event);
			break;
		}
		case SDL_MOUSEBUTTONUP:
		{
			return convert_sdl_mousebuttonup_event(p_sdl_event, p_event);
			break;
		}
	}

	return false;
}

void input_event_process(scene_base *p_scene, real p_frametime)
{
	/* Our SDL event placeholder. */
	SDL_Event event;
	bool pass_event_to_scene = true;
	input_event oge_event;

	/* Grab all the events off the queue. */
	while( SDL_PollEvent( &event ) ) {
		// Convert the event
		if ((p_scene == false) || (event_sdl_to_oge(&event, &oge_event) == false)) {
			// Not a handled event
			continue;
		}

		p_scene->process_event(&oge_event, p_frametime);
	}	
}