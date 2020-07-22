#ifndef __FRAMETIME_H_
#define __FRAMETIME_H_

#include "core_types.h"

void frametime_init();

void frametime_process();

uint32 frametime_get();

uint32 frametime_get_count();

#endif // __FRAMETIME_H_