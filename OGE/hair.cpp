#include "hair.h"

#include "mesh.h"

#include <stdlib.h>

static const unsigned long SEGMENT_VERT_COUNT = 8;
static const unsigned long INDEX_COUNT = 2 * 2 * 6;

void hair:create_segment(unsigned long p_segment_number)
{
	
}

void hair::create_dread(unsigned long p_segment_count)
{
	// Each segment has 8 verts
	unsigned long vert_count = SEGMENT_VERT_COUNT * p_segment_count;

	// index count is 3 verts per triangle, times two tri's per face, times 6 faces

	m_vert_data = (Vector3 *)malloc(sizeof(Vector3) * vert_count);
	m_mesh = (mesh *)malloc(sizeof(mesh));
	m_mesh->m_pos = (Vector3 *)malloc(sizeof(Vector3) * vert_count);
	//m_mesh->m_uv = (uv_coord *)malloc(sizeof(uv_coord) * vert_count);
	m_mesh->m_uv = NULL;

	// Lot's of triangles...
	m_mesh->m_index_count = INDEX_COUNT;
	m_mesh->m_index_buffer = (unsigned long *)malloc(sizeof(unsigned long) * m_mesh->m_index_count);
}

void hair::init()
{
	// Generate a cylinder
	
	// Set usual constraints
	
	// Set fixed constraints only on one end
	
	// Create collision sphere for head
}

void hair::draw()
{

}