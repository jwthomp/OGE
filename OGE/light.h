#ifndef __LIGHT_H_
#define __LIGHT_H_

#include "core_types.h"
#include "transform.h"

class light
{
public:

	typedef enum light_type
	{
		LIGHT_TYPE_NONE,
		LIGHT_TYPE_DIRECTION,
		LIGHT_TYPE_POINT,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_OMNI
	};

	light();
	~light();

	light_type m_type;
	bool m_shadow_casting;

	transform m_transform;
	real m_ambient[4];
	real m_diffuse[4];
	real m_specular[4];

	float m_specular_power;

	float m_constant_attenuation;
	float m_linear_attenuation;
	float m_quadratic_attenuation;

	Vector3 m_spot_direction;
	float m_spot_exponent;
	float m_spot_cos_cutoff;

};

void light_system_init();
void light_system_shutdown();

light * light_create(char *p_name);
void light_release(light *p_light);

uint8 light_get_count();
light * light_get_from_index(uint8 p_index);

#endif /* __LIGHT_H_ */