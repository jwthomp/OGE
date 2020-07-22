#include "scene_base.h"

#include <stdio.h>

void scene_base::init() 
{
	m_scene_layers = NULL;
	m_allow_scene_process = true;
	m_input_state.init();
}

void scene_base::process(real p_frametime)
{
	if (m_scene_layers) {
		m_scene_layers->process(p_frametime);
	}
}

void scene_base::clear_input_state()
{
	m_input_state.clear_for_frame();
}

// Process_events: First pushes events to menus if a related event isn't in the works for the scene (already pushed)
//							Then asks menus if can pass event to scene
//							If not, sets rest state for scene
void scene_base::process_event(input_event *p_event, real p_frametime)
{
	bool allow_scene_to_process = true;

	// Is the scene_base already working with an event related to this?
	if (m_scene_layers && should_pass_event_to_layer(p_event) == true) {
		m_scene_layers->process_event(p_event, p_frametime);
		m_scene_layers->should_pass_event_to_scene(p_event, &allow_scene_to_process);
	}

	if (allow_scene_to_process) {
		handle_event(p_event, p_frametime);
	}
}

void scene_base::scene_layer_add(scene_layer * p_scene_layer)
{
	p_scene_layer->m_next = m_scene_layers;
	m_scene_layers = p_scene_layer;
}

#pragma message(__FILE__ "    TODO: Implement scene_base::should_pass_event_to_layer")
bool scene_base::should_pass_event_to_layer(input_event const* p_event) const
{
	return false;
}

#pragma message(__FILE__ "    TODO: Implement scene_base::handle_event")
void scene_base::handle_event(input_event const* p_event, real p_frametime)
{
	// Store this event in my input structure
	switch(p_event->m_event_type) {
		case INPUT_EVENT_TYPE_KEYDOWN:
			m_input_state.set_key_state(p_event->m_event_keyboard.m_key, true, false, false);
			break;
		case INPUT_EVENT_TYPE_KEYUP:
			m_input_state.set_key_state(p_event->m_event_keyboard.m_key, false, false, false);
			break;
		case INPUT_EVENT_TYPE_MOUSEBUTTONDOWN:
			m_input_state.set_mouse_down(p_event->m_event_mouse_button_down.m_button);
			break;
		case INPUT_EVENT_TYPE_MOUSEBUTTONUP:
			m_input_state.set_mouse_up(p_event->m_event_mouse_button_down.m_button);
			break;
		case INPUT_EVENT_TYPE_JOYAXISMOTION:
			m_input_state.set_joystick_motion(p_event->m_event_joystick_motion.m_joystick_index, 
															p_event->m_event_joystick_motion.m_axis_index, 
															p_event->m_event_joystick_motion.m_axis_value);
			break;
		case INPUT_EVENT_TYPE_JOYBUTTONDOWN:
			m_input_state.set_joystick_button_down(p_event->m_event_joystick_button_down.m_joystick_index,
																p_event->m_event_joystick_button_down.m_button_index);
			break;
		case INPUT_EVENT_TYPE_JOYBUTTONUP:
			m_input_state.set_joystick_button_up(p_event->m_event_joystick_button_up.m_joystick_index,
																p_event->m_event_joystick_button_up.m_button_index);
			break;
		default:
			break;
	};
}