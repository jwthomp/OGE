#ifndef __MATERIAL_H_
#define __MATERIAL_H_

#include "core_types.h"

#include "texture.h"
#include "shader.h"

#define MAX_MATERIAL_NAME_LENGTH (64)



class material
{
public:
	material();
	~material();

	//char m_material_name[MAX_MATERIAL_NAME_LENGTH];
	real32 m_color_ambient[4];
	real32 m_color_diffuse[4];
	real32 m_color_spec[4];
	texture *m_texture;
	shader *m_shader;
};

void material_system_init();
void material_system_shutdown();

material *material_create(char *p_material_name);
void material_release(material *p_mat);

bool material_load(material *p_mat, char *p_texture_name);


#endif /* __MATERIAL_H_ */