#ifndef __INPUT_LIB_H_
#define __INPUT_LIB_H_

#include "core_types.h"
#include "input_state.h"

bool input_lib_init();
void input_lib_process(real p_frametime);
bool input_lib_shutdown();

uint8 input_lib_get_joystick_count();

//input_state const *input_lib_get_input_state_for_frame();

#endif // __INPUT_LIB_H_