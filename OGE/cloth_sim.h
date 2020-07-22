#ifndef __CLOTH_SIM_H_
#define __CLOTH_SIM_H_

#include "particle_system.h"

#include "core_types.h"

class mesh_dynamic;

class cloth_sim
{
public:
	cloth_sim() {}

	virtual void init() = 0;
	
	void simulate(real p_frametime);

protected:
	particle_system m_particle_system;
	constraint *m_constraints;
	unsigned long m_constraint_count;
	Vector3 *m_vert_data;

	virtual void simulate_post() = 0;
};

#endif // __CLOTH_SIM_H_
