#include "physics_lib.h"

#include "cloth_sim.h"

#include <list>

static std::list<cloth_sim *>g_cloth_sims;

void physics_lib_cloth_sim_add(cloth_sim *p_cloth_sim)
{
	g_cloth_sims.push_back(p_cloth_sim);
}

void physics_lib_simulate(real p_frametime)
{
	std::list<cloth_sim *>::iterator cloth_sim_iter;
	for (cloth_sim_iter = g_cloth_sims.begin(); cloth_sim_iter != g_cloth_sims.end(); ++cloth_sim_iter) {
		(*cloth_sim_iter)->simulate(p_frametime);
	}
}