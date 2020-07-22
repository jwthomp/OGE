#ifndef __INPUT_STATE_H_
#define __INPUT_STATE_H_

#include "core_types.h"

#include "input_event.h"

class input_state
{
public:
	struct key_state 
	{
		bool m_key_pressed;
		bool m_key_just_pressed;
		bool m_key_just_released;
	};
	
	key_state const * key_get_state(key p_key) const;

	bool key_is_pressed(key p_key) const;
	bool key_just_pressed(key p_key) const;
	bool key_just_released(key p_key) const;
	
	void mouse_delta(int32 *p_delta_x, int32 *p_delta_y) const;
	void mouse_absolute(uint32 *p_x, uint32 *p_y) const;
	bool mouse_button_pressed(uint8 p_button_index) const;
	
	int16 joystick_axis(uint8 p_joystick_index, uint8 p_axis_index) const;
	bool joystick_button_pressed(uint8 p_joystick_index, uint8 p_button_index) const;
	
	void init();
	void clear_for_frame();
	
	void set_key_state(key p_key, bool p_key_pressed, bool p_key_just_pressed, bool p_key_just_released);
	void set_mouse_down(uint8 p_button);
	void set_mouse_up(uint8 p_button);
	void set_joystick_motion(uint8 p_joystick_index, uint8 p_axis_index, int16 p_value);
	void set_joystick_button_down(uint8 p_joystick_index, uint8 p_button_index);
	void set_joystick_button_up(uint8 p_joystick_index, uint8 p_button_index);
	
protected:
	
	
	struct joystick_state
	{
		int16 m_joystick_axis[256];
		bool m_joystick_button_pressed[256];
		bool m_joystick_button_unpressed[256];
	};
		
	// Store all data here
	key_state m_key_state[256];
		
	int32 m_mouse_delta_x;
	int32 m_mouse_delta_y;
	bool m_mouse_button_pressed[256];
	bool m_mouse_button_unpressed[256];
	
	uint8 m_joystick_count;
	joystick_state m_joysticks[256];
};

#endif /* __INPUT_STATE_H_ */

