#ifndef __PHYSICS_LIB_H_
#define __PHYSICS_LIB_H_

#include "core_types.h"

class cloth_sim;

void physics_lib_cloth_sim_add(cloth_sim *p_cloth_sim);

void physics_lib_simulate(real p_frametime);

#endif // __PHYSICS_LIB_H_