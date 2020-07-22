#ifndef __INPUT_EVENT_PRIVATE_H_
#define __INPUT_EVENT_PRIVATE_H_

#include "input_event.h"

union SDL_Event;

bool event_sdl_to_oge(SDL_Event *p_sdl_event, input_event *p_event);

#endif /* __INPUT_EVENT_PRIVATE_H_ */