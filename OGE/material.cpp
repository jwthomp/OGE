#include "material.h"
#include "render_lib.h"
#include "ref_counted.h"
#include "allocator_array.h"

#include "SDL.h"
#include "SDL_image.h"

#include <string.h>


#define MATERIAL_MAX_NUMBER (256)

static ref_counted<material, MATERIAL_MAX_NUMBER, allocator_array<ref_count_store<material, MATERIAL_MAX_NUMBER>>> g_allocator_material;

material::material()
{
	memset(m_color_spec, 0, 16);
	memset(m_color_diffuse, 0, 16);
	m_texture = NULL;
	m_shader = NULL;
}

material::~material()
{
	if (m_texture) {
		m_texture->unbind();
		texture_release(m_texture);
		m_texture = NULL;
	}

	if (m_shader) {
		shader_release(m_shader);
		m_shader = NULL;
	}
}

void material_system_init()
{
	g_allocator_material.init(MATERIAL_MAX_NUMBER);
}

void material_system_shutdown()
{
	g_allocator_material.shutdown();
}

material *material_create(char *p_material_name)
{
	return g_allocator_material.create(p_material_name);
}

bool material_load(material *p_mat, char *p_texture_name)
{
	p_mat->m_texture = texture_create(p_texture_name);
	p_mat->m_texture->load(p_texture_name);

	return true;
}

void material_release(material *p_mat)
{
	g_allocator_material.release(p_mat);

	if (p_mat->m_texture != NULL) {
		p_mat->m_texture->unbind();
		texture_release(p_mat->m_texture);
	}
}