#include "resource_manager.h"

#include "mesh.h"
#include "render_lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "importer-collada.h"

#ifdef MAC_OS_X
static const char *g_data_path = "OGE-osx.app/Contents/Resources/Data";
#else
static const char *g_data_path = "Data";
#endif

mesh const* resource_manager_get_mesh(char *p_mesh_name)
{
	char buffer[256];
	sprintf(buffer, "%s/%s", g_data_path, p_mesh_name);
	
	FILE *fp;
	fp = fopen(buffer, "r");
	if (fp == NULL) {
		return NULL;
	}
	
	uint32 face_count = 0;
	uint32 vert_count = 0;

	while(!feof(fp)) {
		memset(buffer, 0, 256);
		fgets(buffer, 256, fp);

		if (!strncmp("v ", buffer, 2)) {
			vert_count++;
		} else if (!strncmp("f ", buffer, 2)) {
			face_count++;
		} else {
			continue;
		}
	}

	fclose(fp);

	mesh *mesh_ptr = new mesh;
	mesh_ptr->m_render_block_count = 1;
	mesh_ptr->m_render_blocks = (render_block *)malloc(sizeof(render_block));
	render_block *render_block_ptr = &mesh_ptr->m_render_blocks[0];

	render_block_ptr->m_format = RENDER_LIB_MESH_FORMAT_VA_TRIANGLES;
	render_block_ptr->m_prepared = false;

	render_block_ptr->m_vertex_count = vert_count;
	render_block_ptr->m_pos = (Vector3 *)malloc(sizeof(Vector3) * vert_count);
	//m_mesh->m_uv = (uv_coord *)malloc(sizeof(uv_coord) * vert_count);
	render_block_ptr->m_uv = NULL;
	render_block_ptr->m_normal = (Vector3 *)malloc(sizeof(Vector3) * vert_count);
	render_block_ptr->m_transform.set_identity();

	render_block_ptr->m_uv = (uv_coord *)malloc(sizeof(uv_coord) * render_block_ptr->m_vertex_count);
	memset(render_block_ptr->m_uv, 0, sizeof(uv_coord) * render_block_ptr->m_vertex_count);

	material *render_mat = material_create("fake_material");
	render_block_ptr->m_material = render_mat;

	if (render_block_ptr->m_material->m_texture == NULL) {
		render_mat->m_color_ambient[0] = 0.5f;
		render_mat->m_color_ambient[1] = 0.5f;
		render_mat->m_color_ambient[2] = 0.5f;
		render_mat->m_color_ambient[3] = 0.5f;
		
		render_mat->m_color_diffuse[0] = 0.5f;
		render_mat->m_color_diffuse[1] = 0.5f;
		render_mat->m_color_diffuse[2] = 0.5f;
		render_mat->m_color_diffuse[3] = 0.5f;

		render_mat->m_color_spec[0] = 0.5f;
		render_mat->m_color_spec[1] = 0.5f;
		render_mat->m_color_spec[2] = 0.5f;
		render_mat->m_color_spec[3] = 0.5f;

		material_load(render_mat, "invalid.png");
		render_mat->m_shader = render_lib_get_default_shader();
	}

	// Lot's of triangles...
	render_block_ptr->m_index_count = face_count * 3;
	render_block_ptr->m_index_buffer = (unsigned long *)malloc(sizeof(unsigned long) * render_block_ptr->m_index_count);

	sprintf(buffer, "%s/%s", g_data_path, p_mesh_name);
	fp = fopen(buffer, "r");
	if (fp == NULL) {
		return NULL;
	}

	uint32 vert_index = 0;
	uint32 face_index = 0;
	Vector3 diff;
	while(!feof(fp)) {
		memset(buffer, 0, 256);
		fgets(buffer, 256, fp);

		if (!strncmp("v ", buffer, 2)) {
			sscanf((buffer + 1),"%f%f%f",
					&render_block_ptr->m_pos[vert_index].m_data[0],
					&render_block_ptr->m_pos[vert_index].m_data[1],
					&render_block_ptr->m_pos[vert_index].m_data[2]);

			//render_block_ptr->m_pos[vert_index].data[0] *= .1f;
			//render_block_ptr->m_pos[vert_index].data[1] *= .1f;
			//render_block_ptr->m_pos[vert_index].data[2] *= .1f;

			vert_index++;

		} else if (!strncmp("f ", buffer, 2)) {
			sscanf((buffer + 1),"%u%u%u",
				&render_block_ptr->m_index_buffer[face_index],
				&render_block_ptr->m_index_buffer[face_index + 1],
				&render_block_ptr->m_index_buffer[face_index + 2]);
			face_index += 3;
			render_block_ptr->m_index_buffer[face_index - 3] -= 1,
			render_block_ptr->m_index_buffer[face_index - 2] -= 1;
			render_block_ptr->m_index_buffer[face_index - 1] -= 1;

			Vector3 normal;
			Vector3& a = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[face_index - 3]];
			Vector3& b = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[face_index - 2]];
			Vector3& c = render_block_ptr->m_pos[render_block_ptr->m_index_buffer[face_index - 1]];
			Vector3 x, y;
			x.set(b.m_data[0] - a.m_data[0], b.m_data[1] - a.m_data[1], b.m_data[2] - a.m_data[2]);
			y.set(c.m_data[0] - a.m_data[0], c.m_data[1] - a.m_data[1], c.m_data[2] - a.m_data[2]);
			normal = x.cross(y);

			render_block_ptr->m_normal[face_index - 3] = normal;
			render_block_ptr->m_normal[face_index - 2] = normal;
			render_block_ptr->m_normal[face_index - 1] = normal;

		} else {
			continue;
		}
	}
	
	fclose(fp);
	
	return mesh_ptr;
}

mesh const*resource_manager_get_collada_mesh(char *p_mesh_name)
{
	char buffer[256];
	sprintf(buffer, "%s/%s", g_data_path, p_mesh_name);

	return importer_collada_load(buffer);
}