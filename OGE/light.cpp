#include "light.h"

#include "allocator_array.h"
#include "ref_counted.h"

#define LIGHT_MAX_NUMBER (512)

static ref_counted<light, LIGHT_MAX_NUMBER, allocator_array<ref_count_store<light, LIGHT_MAX_NUMBER>>> g_allocator_light;

light::light()
{
	m_type = LIGHT_TYPE_NONE;
	m_shadow_casting = false;

	m_spot_direction.set(0.0f, 0.0f, 0.0f);
	m_spot_exponent = 1.0f;
	m_spot_cos_cutoff = 1.0f;
	m_specular_power = 16.0f;
}

light::~light()
{
}

void light_system_init()
{
	g_allocator_light.init(LIGHT_MAX_NUMBER);
}

void light_system_shutdown()
{
	g_allocator_light.shutdown();
}

light * light_create(char *p_name)
{
	return g_allocator_light.create(p_name);
}

void light_release(light *p_light)
{
	g_allocator_light.release(p_light);
}

uint8 light_get_count()
{
	return (uint8)g_allocator_light.get_allocated_count();
}

light * light_get_from_index(uint8 p_index)
{
	return g_allocator_light.get_from_index(p_index);
}