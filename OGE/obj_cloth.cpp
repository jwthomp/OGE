#include "obj_cloth.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mesh_instance_dynamic.h"
#include "mesh.h"
#include "resource_manager.h"

#include "render_lib.h"

#include "SDL_OpenGL.h"

void obj_cloth::init()
{
}

void obj_cloth::simulate_post()
{
	// Move over simulation data into the renderable data
	memcpy(m_mesh_instance->m_dynamic_pos, m_vert_data, sizeof(Vector3) * m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count);

}

bool obj_cloth::get_unshared_verts(unsigned long p_face_index_a, unsigned long p_face_index_b, 
											  unsigned long p_face_index_a_verts[3], unsigned long &p_face_a_count, 
											  unsigned long p_face_index_b_verts[3], unsigned long &p_face_b_count)
{
	p_face_a_count = 0;
	p_face_b_count = 0;

	mesh const*mesh = m_mesh_instance->m_mesh;
	render_block const* render_block_ptr = &mesh->m_render_blocks[0];

	for (unsigned long i = 0; i < 3; i++) {
		bool in_common = false;
		for (unsigned long j = 0; j < 3; j++) {
			if (render_block_ptr->m_index_buffer[(p_face_index_a * 3) + i] == render_block_ptr->m_index_buffer[(p_face_index_b * 3) + j]) {
				in_common = true;
				break;
			}
		}

		if (in_common == false) {
			p_face_index_a_verts[p_face_a_count++] = render_block_ptr->m_index_buffer[(p_face_index_a * 3) + i];
		}
	}

	for (unsigned long i = 0; i < 3; i++) {
		bool in_common = false;
		for (unsigned long j = 0; j < 3; j++) {
			if (render_block_ptr->m_index_buffer[(p_face_index_a * 3) + j] == render_block_ptr->m_index_buffer[(p_face_index_b * 3) + i]) {
				in_common = true;
				break;
			}
		}

		if (in_common == false) {
			p_face_index_b_verts[p_face_b_count++] = render_block_ptr->m_index_buffer[(p_face_index_b * 3) + i];
		}
	}

	// If there are no unique verts then this is totally shared
	if ((p_face_a_count == 0) && (p_face_b_count == 0)) {
		return false;
	}

	// If they are all unshared then there is nothing in common
	if ((p_face_a_count == 3) && (p_face_b_count == 3)) {
		return false;
	}

	if (p_face_a_count == 1) {
		return true;
	}

	return false;
}

void obj_cloth::generate_constraints(unsigned long p_face_count)
{
	// Find all constraints
	// First set is three for each triangle	
	mesh const*mesh_ptr = m_mesh_instance->m_mesh;
	render_block *render_block_ptr = &mesh_ptr->m_render_blocks[0];
	m_constraint_count = render_block_ptr->m_index_count;

	// Loop through each triangle and find all triangles that are adjacent
	for (unsigned long i = 0; i < render_block_ptr->m_index_count; i += 3) {
		// Find every face that shares a point with this triangle
		for (unsigned long j = i; j < render_block_ptr->m_index_count; j+=3) {
			unsigned long face_index_a_verts[3];
			unsigned long face_index_b_verts[3];
			unsigned long face_a_count;
			unsigned long face_b_count;

			if (get_unshared_verts(i / 3, j / 3, face_index_a_verts, face_a_count, face_index_b_verts, face_b_count)) {
				// Create a constraint between all of these
				for (unsigned long a_index = 0; a_index < face_a_count; a_index++) {
					for (unsigned long b_index = 0; b_index < face_b_count; b_index++) {
						m_constraint_count++;
					}
				}
			}
		}
	}

	// Allocate constraint memory
	m_constraint_count += 1;
	m_constraints = (constraint *)malloc(sizeof(constraint) * m_constraint_count);
	unsigned long constraint_index = 0;

	
	m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_FIXED;
	m_constraints[constraint_index].m_particle_a_index = 0;
	m_constraints[constraint_index].m_fixed_pos = render_block_ptr->m_pos[0];
	m_fixed_constraint_index = constraint_index;
	m_pos.set(0.0f, 0.0f, 0.0f);
	constraint_index++;

	Vector3 diff;

	for (unsigned long i = 0; i < render_block_ptr->m_index_count; i += 3) {
		m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
		m_constraints[constraint_index].m_particle_a_index = render_block_ptr->m_index_buffer[i];
		m_constraints[constraint_index].m_particle_b_index = render_block_ptr->m_index_buffer[i + 1];
		diff = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i]] - render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i + 1]];
		m_constraints[constraint_index].m_rest_length = diff.len();
		constraint_index++;

		m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
		m_constraints[constraint_index].m_particle_a_index = render_block_ptr->m_index_buffer[i + 1];
		m_constraints[constraint_index].m_particle_b_index = render_block_ptr->m_index_buffer[i + 2];
		diff = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i + 1]] - render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i + 2]];
		m_constraints[constraint_index].m_rest_length = diff.len();
		constraint_index++;

		m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
		m_constraints[constraint_index].m_particle_a_index = render_block_ptr->m_index_buffer[i + 2];
		m_constraints[constraint_index].m_particle_b_index = render_block_ptr->m_index_buffer[i];
		diff = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i + 2]] - render_block_ptr->m_pos[render_block_ptr->m_index_buffer[i]];
		m_constraints[constraint_index].m_rest_length = diff.len();
		constraint_index++;
	}

	// Loop through each triangle and find all triangles that are adjacent and create constraints
	for (unsigned long i = 0; i < render_block_ptr->m_index_count; i += 3) {
		// Find every face that shares a point with this triangle
		for (unsigned long j = i; j < render_block_ptr->m_index_count; j += 3) {
			unsigned long face_index_a_verts[3];
			unsigned long face_index_b_verts[3];
			unsigned long face_a_count;
			unsigned long face_b_count;

			if (get_unshared_verts(i / 3, j / 3, face_index_a_verts, face_a_count, face_index_b_verts, face_b_count)) {
				// Create a constraint between all of these
				for (unsigned long a_index = 0; a_index < face_a_count; a_index++) {
					for (unsigned long b_index = 0; b_index < face_b_count; b_index++) {
						m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
						m_constraints[constraint_index].m_particle_a_index = face_index_a_verts[a_index];
						m_constraints[constraint_index].m_particle_b_index = face_index_b_verts[b_index];
						diff = render_block_ptr->m_pos[face_index_a_verts[a_index]] - render_block_ptr->m_pos[face_index_b_verts[b_index]];
						m_constraints[constraint_index].m_rest_length = diff.len();
						constraint_index++;
					}
				}
			}
		}
	}

	if (constraint_index != m_constraint_count) {
		int a =0;
		a++;
	}
}

bool obj_cloth::set_obj(mesh_instance_dynamic *p_mesh_instance)
{
	m_mesh_instance = p_mesh_instance;

	m_vert_data = (Vector3 *)malloc(sizeof(Vector3) * m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count);

	for (unsigned long i = 0; i < m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count; i++) {
		m_vert_data[i] = m_mesh_instance->m_mesh->m_render_blocks[0].m_pos[i];
	}

	unsigned long face_count = m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count / 3;

	generate_constraints(face_count);

	m_particle_system.init_particle_system(m_mesh_instance->m_mesh->m_render_blocks[0].m_pos, m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count, sizeof(Vector3), m_constraints, m_constraint_count, 1 / 30.0f, 0.9999f);
	
	//Vector3 p_center(0.0f, 0.0f, 0.0f);
	//m_particle_system.add_collision_sphere(p_center,20.0f);

	m_mesh_instance->m_type = RENDER_LIB_MESH_INSTANCE_TYPE_DYNAMIC;
	m_mesh_instance->m_dynamic_pos = (Vector3 *)malloc(sizeof(Vector3) * m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count);
	memcpy(m_mesh_instance->m_dynamic_pos, m_mesh_instance->m_mesh->m_render_blocks[0].m_pos, m_mesh_instance->m_mesh->m_render_blocks[0].m_vertex_count);

	render_lib_mesh_instance_add(m_mesh_instance);

	return true;
}

// Load an object file
bool obj_cloth::load_obj(char *const p_filename)
{
//	FILE *fp = fopen(p_filename, "r");
//	if (fp == NULL) {
//		return false;
//	}

	m_mesh_instance = new mesh_instance_dynamic;
	m_mesh_instance->m_mesh = resource_manager_get_mesh(p_filename);
	
	if (m_mesh_instance->m_mesh == NULL) {
		free(m_mesh_instance);
		return false;
	}
	
	set_obj(m_mesh_instance);
	
	
	return true;
}

void obj_cloth::simulate(float p_frametime)
{
	m_particle_system.simulate(p_frametime, 1.0f, m_pos, m_vert_data);

	// We need to push vert data to the mesh
	simulate_post();
}

void obj_cloth::move(Vector3 const& p_adjust)
{
	m_pos += p_adjust;
}
