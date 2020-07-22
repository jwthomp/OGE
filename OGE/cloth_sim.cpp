#include "cloth_sim.h"

#include "mesh.h"

#include "SDL_OpenGL.h"



void cloth_sim::simulate(real p_frametime)
{
	Vector3 adjust(0.0f, 0.0f, 0.0f);
	m_particle_system.simulate(p_frametime, 1.0f, adjust, m_vert_data);

	// We need to push vert data to the mesh
	simulate_post();

}