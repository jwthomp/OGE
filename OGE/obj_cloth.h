#ifndef __OBJ_CLOTH_H_
#define __OBJ_CLOTH_H_

#include "cloth_sim.h"

class mesh_instance_dynamic;

class obj_cloth : public cloth_sim
{
public:
	virtual void init();

	// Load an object file
	bool load_obj(char *const p_filename);

	bool set_obj(mesh_instance_dynamic *p_mesh_instance);

	void add_force(Vector3 const& p_force) 
	{
		m_particle_system.add_force(p_force);
	}

	void simulate(float p_frametime);

	void reset_force() 
	{
		m_particle_system.reset_force();
	}

	void move(Vector3 const& p_adjust);

private:
	mesh_instance_dynamic *m_mesh_instance;
	Vector3 m_pos;
	unsigned long m_fixed_constraint_index;

	void generate_constraints(unsigned long p_face_count);
	bool get_unshared_verts(unsigned long p_face_index_a, unsigned long p_face_index_b, 
											  unsigned long p_face_index_a_verts[3], unsigned long &p_face_a_count, 
											  unsigned long p_face_index_b_verts[3], unsigned long &p_face_b_count);

	virtual void simulate_post();
};

#endif // __OBJ_CLOTH_H_