#ifndef __SHADER_H_
#define __SHADER_H_

#include "core_types.h"

#define MAX_SHADER_NAME_LENGTH (64)

class shader
{
public:
	shader();
	~shader();

	void activate();

	void load(char const* p_filename);

	uint32 get_location(char const* p_location_name);

	bool m_loaded;

private:
	uint32 m_shader_id;
	uint32 m_fragment_id;
	uint32 m_vertex_id;
};

void shader_system_init();
void shader_system_shutdown();

shader *shader_create(char *p_name);
void shader_release(shader *p_shader);

void shader_enable_fixed_function_pipeline();


#endif /* __SHADER_H_ */