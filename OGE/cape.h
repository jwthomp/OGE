#ifndef __CAPE_H_
#define __CAPE_H_

#include "cloth_sim.h"
#include "mesh_instance_dynamic.h"

class cape : public cloth_sim
{
public:

	static unsigned long const MESH_WIDTH = 16;
	static unsigned long const MESH_HEIGHT = 16;
	static unsigned long const MESH_SIZE = (MESH_WIDTH * MESH_HEIGHT);
	static unsigned long const CONSTRAINTS_NUM = ((MESH_WIDTH - 1) + ((MESH_HEIGHT - 1) * ((MESH_WIDTH - 1) + (MESH_WIDTH) + ((MESH_WIDTH - 1) * 2))) + (2));
	float MESH_SPACING;
	
	cape() {
		MESH_SPACING = 1.6f / MESH_WIDTH;
	}

	virtual void init()
	{
		// Generate mesh and uv's
		m_vert_data = (Vector3 *)malloc(sizeof(Vector3) * MESH_SIZE);
		m_constraints = (constraint *)malloc(sizeof(constraint) * CONSTRAINTS_NUM);
		m_mesh_instance = new mesh_instance_dynamic;
		mesh *mesh_ptr = new mesh;
		m_mesh_instance->m_mesh = mesh_ptr;

		mesh_ptr->m_render_block_count = 1;
		mesh_ptr->m_render_blocks = (render_block *)malloc(sizeof(render_block));
		render_block *render_block_ptr = &mesh_ptr->m_render_blocks[0];

		render_block_ptr->m_pos = (Vector3 *)malloc(sizeof(Vector3) * MESH_SIZE);
		render_block_ptr->m_uv = (uv_coord *)malloc(sizeof(uv_coord) * MESH_SIZE);
		m_mesh_instance->m_dynamic_pos = (Vector3 *)malloc(sizeof(Vector3) * MESH_SIZE);
		render_block_ptr->m_vertex_count = MESH_SIZE;
		

		for (int x = 0; x < MESH_WIDTH; x++) {
			for (int y = 0; y < MESH_HEIGHT; y++) {
				render_block_ptr->m_pos[x + (y * MESH_WIDTH)].m_data[0] = 0.0f + (x * MESH_SPACING);
				render_block_ptr->m_pos[x + (y * MESH_WIDTH)].m_data[1] = 0.0f + (y * MESH_SPACING);
				render_block_ptr->m_pos[x + (y * MESH_WIDTH)].m_data[2] = 0.0f;
		
				render_block_ptr->m_uv[x + (y * MESH_WIDTH)].m_data[0] = x / (float)(MESH_WIDTH - 1);
				render_block_ptr->m_uv[x + (y * MESH_WIDTH)].m_data[1] = y / (float)(MESH_HEIGHT - 1);
			}
		}

		// Generate index buffer
		// 2 indeces for each row, + 2 indices for each (start + 2 degenerate)
		// For each column in the row except the last one add two indices
		render_block_ptr->m_index_count = ((MESH_HEIGHT - 1) * 4) + ((MESH_HEIGHT - 1) * (MESH_WIDTH - 1) * 2);
		render_block_ptr->m_index_buffer = (unsigned long *)malloc(sizeof(unsigned long) * render_block_ptr->m_index_count);

		unsigned long index_count = 0;
		// Go through each row but the last one
		for (unsigned long row = 0; row < MESH_HEIGHT - 1; row++) {
			// Create first two indices
			unsigned long row_base = row * MESH_WIDTH;
			unsigned long row_next = (row + 1) * MESH_WIDTH;

			// Current vert
			render_block_ptr->m_index_buffer[index_count++] = row_base;
			// Vert below current
			render_block_ptr->m_index_buffer[index_count++] = row_next;

		// Go through each column but the last one
		unsigned long  column = 0;
			for (column = 0; column < MESH_WIDTH - 1; column++) {
				// Create two verts
				render_block_ptr->m_index_buffer[index_count++] = row_base + (column + 1);
				render_block_ptr->m_index_buffer[index_count++] = row_next + (column + 1);
			}

			// Create degenerate triangle
			render_block_ptr->m_index_buffer[index_count++] = row_next + column;
			render_block_ptr->m_index_buffer[index_count++] = row_next;

		}
		
		// We need a constrain for every edge?
		// First row
		for (int x = 0; x < MESH_WIDTH - 1; x++) {
			m_constraints[x].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
			m_constraints[x].m_rest_length = MESH_SPACING;
			m_constraints[x].m_particle_a_index = x;
			m_constraints[x].m_particle_b_index = x + 1;
		}
		
		// Calculat length of cross bars to hold fabric together
		float crossbar_length = sqrt((MESH_SPACING * MESH_SPACING) + (MESH_SPACING * MESH_SPACING));
			
		unsigned long constraint_index = MESH_WIDTH - 1;
		for (int x = 0; x < MESH_HEIGHT - 1; x++) {
			// vertical
			for (int row = 0; row < MESH_WIDTH; row++) {
				m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
				m_constraints[constraint_index].m_rest_length = MESH_SPACING;
				m_constraints[constraint_index].m_particle_a_index = row + (x * MESH_WIDTH);
				m_constraints[constraint_index].m_particle_b_index = row + ((x + 1) * MESH_WIDTH);
				constraint_index++;
			}
		
			// horizontal and cross bar
			for (int column = 0; column < MESH_WIDTH - 1; column++) {
				m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
				m_constraints[constraint_index].m_rest_length = MESH_SPACING;
				m_constraints[constraint_index].m_particle_a_index = column + ((x + 1) * MESH_WIDTH);
				m_constraints[constraint_index].m_particle_b_index = column + ((x + 1) * MESH_WIDTH) + 1;
				constraint_index++;
				
				m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
				m_constraints[constraint_index].m_rest_length = crossbar_length;
				m_constraints[constraint_index].m_particle_a_index = column + (x * MESH_WIDTH);
				m_constraints[constraint_index].m_particle_b_index = column + ((x + 1) * MESH_WIDTH) + 1;
				constraint_index++;
				
				m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_RESTLENGTH;
				m_constraints[constraint_index].m_rest_length = crossbar_length;
				m_constraints[constraint_index].m_particle_a_index = column + ((x + 1) * MESH_WIDTH);
				m_constraints[constraint_index].m_particle_b_index = column + (x * MESH_WIDTH) + 1;
				constraint_index++;
			}
			

		}
		//CONSTRAINTS_NUM
		m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_FIXED;
		m_constraints[constraint_index].m_particle_a_index = 0;
		m_constraints[constraint_index].m_fixed_pos = render_block_ptr->m_pos[0];
		constraint_index++;

		m_constraints[constraint_index].m_constraint_type = constraint::CONSTRAINT_TYPE_FIXED;
		m_constraints[constraint_index].m_particle_a_index = MESH_WIDTH - 1;
		m_constraints[constraint_index].m_fixed_pos = render_block_ptr->m_pos[MESH_WIDTH - 1];
		constraint_index++;
		
		m_particle_system.init_particle_system(render_block_ptr->m_pos, MESH_SIZE, sizeof(Vector3), m_constraints, CONSTRAINTS_NUM, 1 / 30.0f, 0.9999f);

		Vector3 sphere(0.5f, 1.0f, -0.5f);
		m_particle_system.add_collision_sphere(sphere, 0.5f); 

		memcpy(m_mesh_instance->m_dynamic_pos, render_block_ptr->m_pos, render_block_ptr->m_vertex_count);
	}

	void add_force(Vector3 const& p_force) 
	{
		m_particle_system.add_force(p_force);
	}

	void reset_force() 
	{
		m_particle_system.reset_force();
	}

	virtual void simulate_post()
	{
	}

private:
	mesh_instance_dynamic *m_mesh_instance;
};


#endif // __CAPE_H_