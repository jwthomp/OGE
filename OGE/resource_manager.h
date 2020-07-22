#ifndef __RESOURCE_MANAGER_H_
#define __RESOURCE_MANAGER_H_

class mesh;

mesh const* resource_manager_get_mesh(char *p_mesh_name);

mesh const* resource_manager_get_collada_mesh(char *p_mesh_name);

#endif // __RESOURE_MANAGER_H_

