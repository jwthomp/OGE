#ifndef __TEXTURE_H_
#define __TEXTURE_H_

#include "core_types.h"

#define MAX_TEXTURE_NAME_LENGTH (256)

class texture
{
public:
	texture();
	~texture();

	void bind(uint32 p_width, uint32 p_height);
	void unbind();
	void activate();
	void load(char *p_texture_name);

	uint8 *m_texture_data;
private:
	uint32 m_texture_id;
};

void texture_system_init();
void texture_system_shutdown();

texture * texture_create(char *p_name);
void texture_release(texture *p_texture);


#endif /* __TEXTURE_H_ */