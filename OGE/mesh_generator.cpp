#include "mesh_generator.h"

#include "mesh.h"
#include "vector3.h"

#include <stdlib.h>
#include <string.h>

// Create a mesh from a list of triangles
mesh *mesh_generator_from_triangles(Vector3 const p_vectors[][3], unsigned long p_triangle_count)
{
	mesh *mesh_ptr = new mesh;
	mesh_ptr->m_render_block_count = 1;
	mesh_ptr->m_render_blocks = (render_block *)malloc(sizeof(render_block));
	render_block *render_block_ptr = &mesh_ptr->m_render_blocks[0];

	render_block_ptr->m_format = RENDER_LIB_MESH_FORMAT_VA_TRIANGLES;
	render_block_ptr->m_vertex_count = p_triangle_count * 3;
	render_block_ptr->m_index_count = p_triangle_count * 3;
	render_block_ptr->m_uv = NULL;
	render_block_ptr->m_normal = NULL;
	render_block_ptr->m_material = NULL;
	render_block_ptr->m_pos = (Vector3 *)malloc(sizeof(Vector3) * render_block_ptr->m_vertex_count);
	render_block_ptr->m_index_buffer = (unsigned long *)malloc(sizeof(unsigned long) * render_block_ptr->m_index_count);

	unsigned long i;
	for (i = 0; i < p_triangle_count; ++i) {
		render_block_ptr->m_pos[(i * 3) + 0] = p_vectors[i][0];
		render_block_ptr->m_pos[(i * 3) + 1] = p_vectors[i][1];
		render_block_ptr->m_pos[(i * 3) + 2] = p_vectors[i][2];
		render_block_ptr->m_index_buffer[(i * 3) + 0] = (i * 3) + 0;
		render_block_ptr->m_index_buffer[(i * 3) + 1] = (i * 3) + 1;
		render_block_ptr->m_index_buffer[(i * 3) + 2] = (i * 3) + 2;
	}

	return mesh_ptr;
}