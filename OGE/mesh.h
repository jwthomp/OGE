#ifndef __MESH_H_
#define __MESH_H_


#include "render_block.h"
#include "mesh_meta_data.h"

class mesh
{
public:
	render_block *m_render_blocks;
	unsigned long m_render_block_count;
	// Material..  (or part of instance?)

	mesh_meta_data m_meta_data;
};

#endif // __MESH_H_