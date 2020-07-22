#include "particle_system.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//#define NULL (0)

// Verlet integration step
void particle_system::verlet() {
	for(uint32 i = 0; i < m_vertex_count; i++) {
		Vector3& x = m_position_data_current[i]; //(Vector3 *)(((unsigned char *)m_simulation_output) + (i * m_stride));
		Vector3 temp = x;
		Vector3& oldx = m_position_data_old[i];
		Vector3 velocity = x - oldx;
		velocity *= m_damp_factor;

		Vector3 gravity(0.0f, -1.0f, 0.0f);

		x += velocity + ((m_force + gravity) * m_timestep * m_timestep);
		oldx = temp;
	}
}

// Implements particles in a box
void particle_system::satisfy_constraints(Vector3 const& p_adjust) {
#define NUM_ITERATIONS (5)
	for(unsigned long j = 0; j < NUM_ITERATIONS; j++) {
	// Collisions

#if 1
	for (unsigned long points = 0; points < m_vertex_count; points++) {
		Vector3& x = m_position_data_current[points];
		Vector3 delta_vec = m_center - x;
		real difference = m_radius - delta_vec.len();

		Vector3 delta_vec_norm = delta_vec / delta_vec.len();

		if (difference > 0) {
			delta_vec_norm *= difference;
			x -= delta_vec_norm;
		}
	}
#endif
	
	
	// Constraints
      for(uint32 i = 0; i < m_constraint_count; i++) {
			constraint const& c = m_constraints[i];
			Vector3& x1 = m_position_data_current[c.m_particle_a_index];
			Vector3& x2 = m_position_data_current[c.m_particle_b_index];

			switch (c.m_constraint_type) {
				case constraint::CONSTRAINT_TYPE_FIXED:
					x1 = c.m_fixed_pos;
					x1 += p_adjust;
					break;
				case constraint::CONSTRAINT_TYPE_RESTLENGTH:
				{
					real restlength = c.m_rest_length;
					Vector3 delta = x2 - x1;

					delta *= (restlength * restlength) / ((delta * delta) + (restlength * restlength)) - 0.5f;
					x1 -= delta;
					x2 += delta;
				}
					break;
				default:
				break;
			};
		}
   }

   // Constrain one particle of the cloth to origo
   //m_x[0] = Vector3(0,0,0);

	// Do a box constraint
	for(uint32 i = 0; i < m_vertex_count; i++) { // For all particles
		Vector3& x = m_position_data_current[i];
		x = vmin(vmax(x, Vector3(-30.0f, -30.0f, -30.0f)), Vector3(30.0f, 30.0f, 30.0f));
	}
}

particle_system::particle_system() 
{
	m_vertex_data = NULL;
	m_vertex_count = 0;
	m_stride = 0;
	m_time_accumulated = 0.0f;
	m_position_data_current = NULL;
	m_position_data_old = NULL;
	m_constraint_count = 0;
	m_force.m_data[0] = 0.0f;
	m_force.m_data[1] = 0.0f;
	m_force.m_data[2] = 0.0f;
};

particle_system::~particle_system()
{
	if (m_position_data_current) {
		free(m_position_data_current);
		m_position_data_current = NULL;
	}

	if (m_position_data_old) {
		free(m_position_data_old);
		m_position_data_old = NULL;
	}
}

// Initializes a particle system with a pointer to base vertex data
void particle_system::init_particle_system(void const*p_vertex_data, uint32 p_vertex_count, uint32 p_stride, constraint const*p_constraints, uint32 p_constraint_count, real p_timestep, real p_damp_factor)
{
	m_vertex_data = p_vertex_data;
	m_vertex_count = p_vertex_count;
	m_stride = p_stride;
	m_time_accumulated = 0.0f;
	m_position_data_current = (Vector3 *)malloc(sizeof(Vector3) * m_vertex_count);
	m_position_data_old = (Vector3 *)malloc(sizeof(Vector3) * m_vertex_count);
	m_constraint_count = p_constraint_count;
	m_constraints = p_constraints;
	m_timestep = p_timestep;
	m_damp_factor = p_damp_factor;
	
	//////////
	// NOTE: JWT: This code is not safe if vector components are doubles instead of floats
	///////////

	// Fill out current state of data
	unsigned long i;
	for (i = 0; i < p_vertex_count; i++) {
		memcpy(m_position_data_current[i].m_data, ((unsigned char *)p_vertex_data) + (m_stride * i), sizeof(float) * 3);
		memcpy(m_position_data_old[i].m_data, ((unsigned char *)p_vertex_data) + (m_stride * i), sizeof(float) * 3);
	}
}

void particle_system::adjust_to_weight(float p_simulation_weight, Vector3 const& p_adjust)
{
	for(uint32 i = 0; i < m_vertex_count; i++) {
		Vector3& x = m_position_data_current[i]; //(Vector3 *)(((unsigned char *)m_simulation_output) + (i * m_stride));
		
		Vector3 orig = *(Vector3 *)(((unsigned char *)m_vertex_data) + (i * m_stride)) + p_adjust;

		x = orig + ((x - orig) * p_simulation_weight);
	}
}

// Simulate the particle system
void particle_system::simulate(real p_time, real p_simulation_weight, Vector3 const& p_adjust, void *p_simulation_output)
{
	m_simulation_output = p_simulation_output;

	// Find out how much time we have to simulate all together
	real total_time = p_time + m_time_accumulated;

	if (m_stride != sizeof(Vector3)) {
		memcpy((unsigned char *)m_simulation_output, (unsigned char *)m_vertex_data, m_vertex_count * m_stride);
	}

	// Loop through simulation steps
	while(total_time >= m_timestep) {
		verlet();
		satisfy_constraints(p_adjust);
		adjust_to_weight(p_simulation_weight, p_adjust);
		
		//////////
		// NOTE: JWT: This code is not safe if vector components are doubles instead of floats
		///////////

		unsigned long i;
		for (i = 0; i < m_vertex_count; i++) {
			memcpy(((unsigned char *)m_simulation_output) + (m_stride * i), m_position_data_current[i].m_data, sizeof(float) * 3);
		}

		// Subtract out the time we just simulated
		total_time -= m_timestep;
	}

	// Save out any unused time
	m_time_accumulated = total_time;
}



// Add a force to the particle system. Can be used for wind, gravity, etc.
void particle_system::add_force(Vector3 const&p_force)
{
	m_force += p_force;
}

void particle_system::reset_force()
{
	m_force.m_data[0] = 0.0f;
	m_force.m_data[1] = 0.0f;
	m_force.m_data[2] = 0.0f;
}

void particle_system::add_constraints(constraint const*p_constraints, uint32 p_constraint_count) {}
void particle_system::add_collision_primitive(void *p_primitive, uint32 p_primitive_count) {}

void particle_system::add_collision_sphere(Vector3 p_center, real p_radius)
{
	m_center = p_center;
	m_radius = p_radius;
}