#ifndef __MESH_INSTANCE_H_
#define __MESH_INSTANCE_H_

#include "render_lib_types.h"
#include "transform.h"



class mesh;

class mesh_instance
{
public:
	mesh_instance_type m_type;

	mesh const*m_mesh;
	
	mesh_instance();

	transform m_transform;

};

#endif // __MESH_INSTANCE_H_