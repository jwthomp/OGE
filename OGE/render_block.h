#ifndef __RENDER_BLOCK_H_
#define __RENDER_BLOCK_H_

#include "vector3.h"
#include "render_lib_types.h"
#include "matrix.h"
#include "material.h"


class uv_coord
{
public:
	float m_data[2];
};

class render_block
{
public:
	render_block();

	mesh_format m_format;
	unsigned long m_index_count;
	unsigned long *m_index_buffer;
	unsigned long m_vertex_count;
	Vector3 *m_pos;
	uv_coord *m_uv;
	Vector3 *m_normal;
	matrix44 m_transform;
	material const*m_material;
	bool m_prepared;


	void prepare();
	void draw();

	uint32 m_display_list_id;
};

#endif /* __RENDER_BLOCK_H_ */