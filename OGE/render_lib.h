#ifndef __RENDER_LIB_H_
#define __RENDER_LIB_H_

#include "render_lib_types.h"

#include "vector3.h"
#include "quaternion.h"
#include "render_block.h"
#include "mesh_instance_dynamic.h"
#include "camera.h"

class mesh_instance;
class shader;

extern camera g_camera;

bool render_lib_init(unsigned long p_width, unsigned long p_height);

mesh_id render_lib_mesh_instance_add(mesh_instance *p_mesh_instance);
void render_lib_mesh_instance_remove(mesh_instance *p_mesh_instance);

void render_lib_set_camera(Vector3 const& p_pos, quaternion const& p_orient);

void render_lib_render_block(render_block *p_render_block, mesh_instance_dynamic *p_dynamic_mesh);

void render_lib_render();

unsigned long render_texture_bind(void *p_pixel_data, unsigned long p_h, unsigned long p_w);

void render_texture_unbind(unsigned long p_texture_id);

void render_lib_set_default_shader(char *p_shader_name);
shader * render_lib_get_default_shader();

#endif // __RENDER_LIB_H_