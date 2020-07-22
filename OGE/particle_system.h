#ifndef __PARTICLE_SYSTEM_H_
#define __PARTICLE_SYSTEM_H_

#include "Vector3.h"

#include "core_types.h"

struct constraint {
   uint32 m_particle_a_index;
	uint32 m_particle_b_index;

	typedef enum constraint_types {
		CONSTRAINT_TYPE_FIXED,
		CONSTRAINT_TYPE_RESTLENGTH,
		CONSTRAINT_TYPE_ANGLE,
	};

	constraint_types m_constraint_type;
   real m_rest_length;
	Vector3 m_fixed_pos;
	real m_angle;
};



class particle_system
{
public:

	particle_system();

	~particle_system();

	// Initializes a particle system with a pointer to base vertex data
	void init_particle_system(void const*p_vertex_data, uint32 p_vertex_count, uint32 p_stride, constraint const*p_constraints, uint32 p_constraint_count, real p_timestep, real p_damp_factor);

	// Simulate the particle system
	void simulate(real p_time, real p_simulation_weight, Vector3 const& p_adjust, void *p_simulation_output);

	// Add a force to the particle system. Can be used for wind, gravity, etc.
	void add_force(Vector3 const&p_force);
	
	void reset_force();

	void add_constraints(constraint const*p_constraints, uint32 p_constraint_count);
	void add_collision_primitive(void *p_primitive, uint32 p_primitive_count);
	
	void add_collision_sphere(Vector3 p_center, real p_radius);

private:
	void const*m_vertex_data;
	uint32 m_vertex_count;
	uint32 m_stride;

	real m_time_accumulated;
	Vector3 *m_position_data_current;
	Vector3 *m_position_data_old;
	constraint const*m_constraints;
	uint32 m_constraint_count;
	real m_timestep;
	Vector3 m_force;
	void *m_simulation_output;
	real m_damp_factor;
	
	Vector3 m_center;
	real m_radius;

	// Verlet integration step
	void verlet();

	// Implements particles in a box
	void satisfy_constraints(Vector3 const& p_adjust);

	void adjust_to_weight(real p_simulation_weight, Vector3 const& p_adjust);
};

#endif // __PARTICLE_SYSTEM_H_