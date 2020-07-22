#ifndef __INPUT_EVENT_H_
#define __INPUT_EVENT_H_

#include "core_types.h"

class scene_base;

typedef uint8 key;

const key KEY_TILDE				= 0;
const key KEY_1					= 1;
const key KEY_2					= 2;
const key KEY_3					= 3;
const key KEY_4					= 4;
const key KEY_5					= 5;
const key KEY_6					= 6;
const key KEY_7					= 7;
const key KEY_8					= 8;
const key KEY_9					= 9;
const key KEY_0					= 10;
const key KEY_MINUS				= 11;
const key KEY_EQUALS				= 12;
const key KEY_BACKSPACE			= 13;
const key KEY_TAB					= 14;
const key KEY_Q					= 15;
const key KEY_W					= 16;
const key KEY_E					= 17;
const key KEY_R					= 18;
const key KEY_T					= 19;
const key KEY_Y					= 20;
const key KEY_U					= 21;
const key KEY_I					= 22;
const key KEY_O					= 23;
const key KEY_P					= 24;
const key KEY_LEFTBRACKET		= 25;
const key KEY_RIGHTBRACKET		= 26;
const key KEY_BACKSLASH			= 27;
const key KEY_CAPSLOCK			= 28;
const key KEY_A					= 29;
const key KEY_S					= 30;
const key KEY_D					= 31;
const key KEY_F					= 32;
const key KEY_G					= 33;
const key KEY_H					= 34;
const key KEY_J					= 35;
const key KEY_K					= 36;
const key KEY_L					= 37;
const key KEY_SEMICOLON			= 38;
const key KEY_QUOTE				= 39;
const key KEY_ENTER				= 40;
const key KEY_LSHIFT				= 41;
const key KEY_Z					= 42;
const key KEY_X					= 43;
const key KEY_C					= 44;
const key KEY_V					= 45;
const key KEY_B					= 46;
const key KEY_N					= 47;
const key KEY_M					= 48;
const key KEY_COMMA				= 49;
const key KEY_PERIOD				= 50;
const key KEY_FORWARDSLASH		= 51;
const key KEY_RSHIFT				= 52;
const key KEY_LCONTROL			= 53;
const key KEY_LALT				= 54;
const key KEY_SPACE				= 55;
const key KEY_RALT				= 56;
const key KEY_RCONTROL			= 57;
const key KEY_UPARROW			= 58;
const key KEY_LARROW				= 59;
const key KEY_DOWNARROW			= 60;
const key KEY_RARRAOW			= 61;
const key KEY_ESCAPE				= 62;

typedef enum input_event_type {
	INPUT_EVENT_TYPE_KEYDOWN,
	INPUT_EVENT_TYPE_KEYUP,
	INPUT_EVENT_TYPE_JOYAXISMOTION,
	INPUT_EVENT_TYPE_JOYBUTTONDOWN,
	INPUT_EVENT_TYPE_JOYBUTTONUP,
	INPUT_EVENT_TYPE_MOUSEMOTION,
	INPUT_EVENT_TYPE_MOUSEBUTTONDOWN,
	INPUT_EVENT_TYPE_MOUSEBUTTONUP
};

struct input_event_keyboard
{
	uint8 m_event_type;
	key m_key;
};

struct input_event_mouse_motion
{
	uint8 m_event_type;
	uint16 m_axis_x;
	uint16 m_axis_y;
	int16 m_axis_relx;
	int16 m_axis_rely;
};

struct input_event_mouse_button_up
{
	uint8 m_event_type;
	uint8 m_button;
};

struct input_event_mouse_button_down
{
	uint8 m_event_type;
	uint8 m_button;
};

struct input_event_joystick_motion
{
	uint8 m_event_type;
	uint8 m_joystick_index;
	uint8 m_axis_index;
	int16 m_axis_value;
};

struct input_event_joystick_button_down
{
	uint8 m_event_type;
	uint8 m_joystick_index;
	uint8 m_button_index;
};

struct input_event_joystick_button_up
{
	uint8 m_event_type;
	uint8 m_joystick_index;
	uint8 m_button_index;
};

union input_event
{
	uint8 m_event_type;
	input_event_keyboard m_event_keyboard;
	input_event_mouse_motion m_event_mouse_motion;
	input_event_mouse_button_up m_event_mouse_button_up;
	input_event_mouse_button_down m_event_mouse_button_down;
	input_event_joystick_motion m_event_joystick_motion;
	input_event_joystick_button_down m_event_joystick_button_down;
	input_event_joystick_button_up m_event_joystick_button_up;
};


void input_event_process(scene_base *p_scene, real p_frametime);

#endif /* __INPUT_EVENT_H_ */