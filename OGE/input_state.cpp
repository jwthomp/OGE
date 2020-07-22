#include "input_state.h"

#include "input_lib.h"

#include <string.h>


void input_state::init()
{
	// Clear all input state out to rest values
		// Store all data here
	memset(m_key_state, 0, sizeof(key_state) * 256);
	m_mouse_delta_x = 0;
	m_mouse_delta_y = 0;
	memset(m_mouse_button_pressed, 0, sizeof(bool) * 256);
	memset(m_mouse_button_unpressed, 0, sizeof(bool) * 256);
	memset(m_joysticks, 0, sizeof(joystick_state) * 256);

	m_joystick_count = input_lib_get_joystick_count();
}

void input_state::clear_for_frame()
{
	// Flip the pressed state of any buttons that have been unpressed. We do this in case a press and unpress come in the same frame
	for (uint16 i = 0; i < 256; i++) { 
		if (m_mouse_button_unpressed[i]) {
			m_mouse_button_unpressed[i] = false;
			m_mouse_button_pressed[i] = false;
		}
	}

	for (uint16 i = 0; i < m_joystick_count; i++) {
		for(uint16 j = 0; j < 256; j++) {
			if (m_joysticks[i].m_joystick_button_unpressed[j]) {
				m_joysticks[i].m_joystick_button_pressed[j] = false;
				m_joysticks[i].m_joystick_button_unpressed[j] = false;
			}
		}
	}

	m_joystick_count = input_lib_get_joystick_count();
}


void input_state::set_mouse_down(uint8 p_button)
{
	m_mouse_button_pressed[p_button] = true;
}

void input_state::set_mouse_up(uint8 p_button)
{
	m_mouse_button_unpressed[p_button] = false;
}

void input_state::set_key_state(key p_key, bool p_key_pressed, bool p_key_just_pressed, bool p_key_just_released)
{
	m_key_state[p_key].m_key_pressed = p_key_pressed;
	m_key_state[p_key].m_key_just_pressed = p_key_just_pressed;
	m_key_state[p_key].m_key_just_released = p_key_just_released;
}

void input_state::set_joystick_motion(uint8 p_joystick_index, uint8 p_axis_index, int16 p_value)
{
	m_joysticks[p_joystick_index].m_joystick_axis[p_axis_index] = p_value;
}

void input_state::set_joystick_button_down(uint8 p_joystick_index, uint8 p_button_index)
{
	m_joysticks[p_joystick_index].m_joystick_button_pressed[p_button_index] = true;
}

void input_state::set_joystick_button_up(uint8 p_joystick_index, uint8 p_button_index)
{
	m_joysticks[p_joystick_index].m_joystick_button_unpressed[p_button_index] = true;
}

input_state::key_state const * input_state::key_get_state(key p_key) const
{
	return &m_key_state[p_key];
}

bool input_state::key_is_pressed(key p_key) const
{
	return m_key_state[p_key].m_key_pressed;
}

bool input_state::key_just_pressed(key p_key) const
{
	return m_key_state[p_key].m_key_just_pressed;
}

bool input_state::key_just_released(key p_key) const
{
	return m_key_state[p_key].m_key_just_released;
}

void input_state::mouse_delta(int32 *p_delta_x, int32 *p_delta_y) const
{
}


void input_state::mouse_absolute(uint32 *p_x, uint32 *p_y) const
{
}

bool input_state::mouse_button_pressed(uint8 p_button_index) const
{
	return m_mouse_button_pressed[p_button_index];
}

int16 input_state::joystick_axis(uint8 p_joystick_index, uint8 p_axis_index) const
{
	return m_joysticks[p_joystick_index].m_joystick_axis[p_axis_index];
}

bool input_state::joystick_button_pressed(uint8 p_joystick_index, uint8 p_button_index) const
{
	return m_joysticks[p_joystick_index].m_joystick_button_pressed[p_button_index];
}

