#ifndef __FRAMEBUFFER_OBJECT_H_
#define __FRAMEBUFFER_OBJECT_H_

#include "core_types.h"

class framebuffer_object
{
public:
	typedef uint32 framebuffer_format;
	static framebuffer_format const FRAMEBUFFER_FORMAT_DEPTH16 = 1;
	static framebuffer_format const FRAMEBUFFER_FORMAT_DEPTH24 = 2;
	static framebuffer_format const FRAMEBUFFER_FORMAT_DEPTH32 = 4;

	framebuffer_object();
	~framebuffer_object();

	void init(uint32 p_width, uint32 p_height, framebuffer_format p_format);
	bool is_status_ready() const;
	void set_target_texture(uint32 p_texture_handle, uint8 p_color_buffer);
	void bind() const;
	void unbind() const;

	uint32 get_width() const;
	uint32 get_height() const;

	uint32 get_depth_buffer_id() const;

private:
	uint32 m_width;
	uint32 m_height;

	// This is OpenGL specific data, will likely want to make this private at some point.
	uint32 m_frame_buffer_index;
	uint32 m_depth_buffer_index;
};

void framebuffer_object_system_init();
void framebuffer_object_system_shutdown();

framebuffer_object * framebuffer_object_create();
void framebuffer_object_destroy(framebuffer_object *p_framebuffer_object);

#endif /* __FRAMEBUFFER_OBJECT_H_ */