#ifndef __MESH_INSTANCE_DYNAMIC_H_
#define __MESH_INSTANCE_DYNAMIC_H_

#include "mesh_instance.h"

class mesh_instance_dynamic : public mesh_instance
{
public:
	Vector3 *m_dynamic_pos;
};

#endif // __MESH_INSTANCE_DYNAMIC_H_