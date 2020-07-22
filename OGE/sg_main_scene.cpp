#include "sg_main_scene.h"

#include "Vector3.h"
#include "quaternion.h"
#include "render_lib.h"
#include "input_state.h"
#include "matrix.h"
#include "mesh.h"
#include "mesh_instance.h"
#include "resource_manager.h"	
#include "objects_guff.h"
#include "light.h"

#include <stdlib.h>

#define BASE_OFFSET (-10000.0f)
#define CAM_OFFSET (3000.0f)
#define CAM_UOFFSET (500.0f)

extern bool g_swap;

static Vector3 g_camera_pos(0.0f, 0.0f, BASE_OFFSET);
static quaternion g_camera_orient;
static mesh const* g_ship_mesh;
static mesh_instance *g_ship_mesh_instance;
static mesh_instance *g_ship_mesh_instance2;

static light *g_light;
static light *g_light2;

static light *g_light_array[512];

static bool UPDATE = true;

static void create_tod_light()
{
	char str[64];
	for (uint32 i = 0; i < 49; ++i) {
		sprintf(str, "tod_light%d", i + 3);
		g_light_array[i] = light_create(str);
		g_light_array[i]->m_type = light::LIGHT_TYPE_POINT;
		g_light_array[i]->m_ambient[0] = 0.0f;
		g_light_array[i]->m_ambient[1] = 0.0f;
		g_light_array[i]->m_ambient[2] = 0.0f;
		g_light_array[i]->m_ambient[3] = 0.0f;
		g_light_array[i]->m_diffuse[0] = 1.0f;
		g_light_array[i]->m_diffuse[1] = 1.0f;
		g_light_array[i]->m_diffuse[2] = 1.0f;
		g_light_array[i]->m_diffuse[3] = 1.0f;
		g_light_array[i]->m_specular[0] = 0.2f;
		g_light_array[i]->m_specular[1] = 0.2f;
		g_light_array[i]->m_specular[2] = 0.2f;
		g_light_array[i]->m_specular[3] = 0.2f;
		g_light_array[i]->m_shadow_casting = false;
	}

#if 0
	g_light = light_create("tod_light");
	g_light->m_type = light::LIGHT_TYPE_POINT;
	g_light->m_ambient[0] = 0.1f;
	g_light->m_ambient[1] = 0.1f;
	g_light->m_ambient[2] = 0.1f;
	g_light->m_ambient[3] = 1.0f;
	g_light->m_diffuse[0] = 1.0f;
	g_light->m_diffuse[1] = 1.0f;
	g_light->m_diffuse[2] = 1.0f;
	g_light->m_diffuse[3] = 1.0f;
	g_light->m_specular[0] = 0.0f;
	g_light->m_specular[1] = 0.0f;
	g_light->m_specular[2] = 0.0f;
	g_light->m_specular[3] = 1.0f;

	g_light->m_specular_power = 16.0f;

	g_light->m_constant_attenuation = 1.0f;
	g_light->m_linear_attenuation = 0.0000f;
	g_light->m_quadratic_attenuation = 0.0f;
	g_light->m_shadow_casting = false;

	g_light->m_spot_direction.set(0.0f, 0.0f, 0.0f);
	g_light->m_spot_exponent = 4.000f;
	g_light->m_spot_cos_cutoff = 0.4f;
#endif

#if 1
	g_light2 = light_create("tod_light2");
	g_light2->m_type = light::LIGHT_TYPE_POINT;
	g_light2->m_ambient[0] = 0.1f;
	g_light2->m_ambient[1] = 0.1f;
	g_light2->m_ambient[2] = 0.1f;
	g_light2->m_ambient[3] = 1.0f;
	g_light2->m_diffuse[0] = 1.0f;
	g_light2->m_diffuse[1] = 1.0f;
	g_light2->m_diffuse[2] = 1.0f;
	g_light2->m_diffuse[3] = 1.0f;
	g_light2->m_specular[0] = 1.0f;
	g_light2->m_specular[1] = 1.0f;
	g_light2->m_specular[2] = 1.0f;
	g_light2->m_specular[3] = 1.0f;

	g_light2->m_specular_power = 128.0f;

	g_light2->m_constant_attenuation = 1.0f;
	g_light2->m_linear_attenuation = 0.0000f;
	g_light2->m_quadratic_attenuation = 0.0000001f;
	g_light2->m_shadow_casting = false;

	g_light2->m_spot_direction.set(0.0f, 0.0f, 1.0f);
	g_light2->m_spot_exponent = 16.0f;
	g_light2->m_spot_cos_cutoff = 0.8f;

	Vector3 pos(0.0f, 1.0f, 0.0f);
	quaternion qt;
	qt.CreateFromAngles(0.0f, 0.0f, 0.0f);
	g_light2->m_transform.set_values(&pos, &qt, NULL);


#endif
}

void sg_main_scene::init()
{
	scene_base::init();
	m_xvel = 0.0f;
	m_xrot = 0.0f;
	m_yrot = 0.0f;

	mesh const*ground_plane = resource_manager_get_collada_mesh("ground_plane.dae");
	mesh_instance *gp_mi = objects_guff_create_instance(ground_plane, 0.0f, 1000.0f, 0.0f, 0.0f, 0.0f, 0.0f); //, 50.0f, 1.0f, 50.0f);

	//g_ship_mesh = resource_manager_get_collada_mesh("starship01_nowing.dae");
	g_ship_mesh = resource_manager_get_collada_mesh("car.dae");

#define ROTATE_MODEL
#if defined(ROTATE_MODEL)
	quaternion q1, q2;
	q1.CreateFromAxisAngle(0.0f, 1.0f, 0.0f, 180.0f);
	q2.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, 80.0f);
	q1 = q1 * q2;
	matrix44 transform_matrix;
	q1.CreateMatrix(transform_matrix.m_data);

	for (uint32 i = 0; i < g_ship_mesh->m_render_block_count; i++) {
		for (uint32 j = 0; j < g_ship_mesh->m_render_blocks[i].m_vertex_count; j++) {
			Vector3 out = transform_matrix * g_ship_mesh->m_render_blocks[i].m_pos[j];
			g_ship_mesh->m_render_blocks[i].m_pos[j].set(out.m_data[0], out.m_data[1], out.m_data[2]);
		}
	}
#endif

	//g_ship_mesh = resource_manager_get_collada_mesh("ship_aligned.dae");
	g_ship_mesh_instance = objects_guff_create_instance(g_ship_mesh, 0.0f, 0.0f, -9000.0f, 0.0f, 0.0f, 0.0f, 10.0f, 10.0f, 10.0f);


	mesh const*box = resource_manager_get_collada_mesh("temple.dae");

	g_ship_mesh_instance2 = objects_guff_create_instance(box, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 25.0f, 25.0f, 25.0f);

	
	
	for (uint32 i = 0; i < 0; i++) {
		g_ship_mesh_instance = objects_guff_create_instance(g_ship_mesh, i * 100.0f, i * 100.0f, 0.0f);

		//quaternion quat;
		//quat.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, i);
		//Vector3 scale;
		//scale.set(1.0f, 1.0f, 1.0f);
		//scale = scale * 0.1f * i;
		//g_ship_mesh_instance->set_values(NULL, &quat, &scale);
	}	
	
	create_tod_light();
}

void sg_main_scene::process(real p_frametime)
{
	scene_base::process(p_frametime);
	
	if (m_allow_scene_process) {
				
		process_input(p_frametime);
		
		render_lib_set_camera(g_camera_pos, g_camera_orient);

		if (g_light) {
			g_light->m_transform.set_values(&g_camera_pos, &g_camera_orient, NULL);
			{
				//Vector3 pos(0.0f, 0.0f, BASE_OFFSET);
				//g_light->m_transform.set_values(&pos, NULL, NULL);


					g_light->m_spot_direction = g_ship_mesh_instance->m_transform.m_transform_matrix.get_fvec(); //g_ship_mesh_instance->m_transform.get_position() - pos - ;
			}
		}

		//g_light2->m_transform.set_values(&g_camera_pos, &g_camera_orient, NULL);
		//g_light2->m_spot_direction = g_light2->m_transform.m_transform_matrix.get_fvec();

		if (UPDATE) {
			//g_ship_mesh_instance2->m_transform.set_values(&g_camera_pos, &g_camera_orient, NULL);

			//Vector3 val(0.0f, 0.0f, 0.0f);
			//g_ship_mesh_instance2->m_transform.set_values(&val, NULL, NULL);

			
		}
	}
}

void sg_main_scene::process_input(real p_frametime)
{
	static float cur_vel = 0.0f;

	// Test for input and act on it
	if (m_input_state.key_is_pressed(KEY_ESCAPE)) {
		exit(0);
	}

	extern shader *g_shader_lighting;

	if (m_input_state.key_is_pressed(KEY_W)) {
		//UPDATE = true;
		//g_light->m_specular[0] = 1.0f;
	//g_light->m_specular[1] = 1.0f;
	//g_light->m_specular[2] = 1.0f;
		//g_light2->m_type = light::LIGHT_TYPE_POINT;
		//g_swap = true;
		//g_shader_lighting = shader_create("deferred_lighting_nospec");
	g_light2->m_spot_exponent = 16.0f;
#if 0
		if (g_light2->m_type == light::LIGHT_TYPE_NONE) {
			g_light2->m_type = light::LIGHT_TYPE_POINT;
		} else {
			g_light2->m_type = light::LIGHT_TYPE_NONE;
		}
#endif
	}

	if (m_input_state.key_is_pressed(KEY_S)) {
		//UPDATE = false;
		//g_light->m_specular[0] = 0.0f;
		//g_light->m_specular[1] = 0.0f;
		//g_light->m_specular[2] = 0.0f;
		//g_light2->m_type = light::LIGHT_TYPE_DIRECTION;
		//g_swap = false;
		//g_shader_lighting = shader_create("deferred_lighting");
	g_light2->m_spot_exponent = 0.0f;
	}

	static bool stop = false;
	if (m_input_state.key_is_pressed(KEY_A)) {
		g_light2->m_type = light::LIGHT_TYPE_POINT;
		if (g_light->m_type == light::LIGHT_TYPE_NONE) {
			//g_light->m_type = light::LIGHT_TYPE_POINT;
		} else {
			//g_light->m_type = light::LIGHT_TYPE_NONE;
		}

	}

	if (m_input_state.mouse_button_pressed(4)) {
		m_xvel += 1.0f;
	} else if (m_input_state.mouse_button_pressed(5)) {
		m_xvel -= 1.0f;
	}

	if (m_input_state.joystick_button_pressed(0, 2)) {
		stop = true;
	} else if (m_input_state.joystick_button_pressed(0, 3)) {
		stop = false;
	}

	if (m_input_state.joystick_button_pressed(0, 0)) {
		m_xvel += 100.0f;
	} else if (m_input_state.joystick_button_pressed(0, 1)) {
		m_xvel -= 100.0f;
	} else {
		m_xvel = 0.0f;
	}

	if (m_input_state.key_is_pressed(KEY_D)) {
		//g_light2->m_type = light::LIGHT_TYPE_DIRECTION;
		m_xvel = -cur_vel;
	}
	
	if (m_input_state.joystick_button_pressed(0, 7)) {
		exit(0);
	}
	

	cur_vel += m_xvel;
	
	
	//g_camera_orient.CreateMatrix(matrix.m_data);
	
	//
	// TODO: Change in orientation should be based on fvec, not fixed angles
	//

	float joy0_axis_x = m_input_state.joystick_axis(0, 1) / 32768.0f;
	float joy0_axis_y = m_input_state.joystick_axis(0, 0) / 32768.0f;


	m_xrot = joy0_axis_x * p_frametime * 100.0f;
	m_yrot = joy0_axis_y * p_frametime * 100.0f;

	quaternion cur = g_ship_mesh_instance->m_transform.get_orient();

	quaternion a, b;
	a.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, m_xrot);
	b.CreateFromAxisAngle(0.0f, 0.0f, 1.0f, m_yrot);
	
	quaternion val = cur * b * a;

	if (stop == false) {
		g_ship_mesh_instance->m_transform.set_values(NULL, &val, NULL);
	}

	Vector3 adjust;
	matrix44 orient;
	val.CreateMatrix(orient.m_data);
	Vector3 fvecN = orient.get_fvec();
	fvecN = fvecN / fvecN.len();
	
	Vector3 fvec = fvecN * cur_vel * p_frametime;
	
	Vector3 new_pos = g_ship_mesh_instance->m_transform.get_position() + fvec;

	if (stop == false) {
		g_ship_mesh_instance->m_transform.set_values(&new_pos, &val, NULL);
	}

	// Construct camera
	if (1) {
		Vector3 uvecN = orient.get_uvec();
		uvecN = uvecN / uvecN.len();
		*(Vector3 *)(&orient._03) = new_pos - (fvecN * CAM_OFFSET) - (uvecN * CAM_UOFFSET);
		g_camera.set_transform(&orient);

		if (stop == false) {
			Vector3 pos(0.0f, 0.0f, BASE_OFFSET);
			g_camera_pos = new_pos + (fvecN * CAM_OFFSET);
			g_camera_orient = val;
		}
	}
}