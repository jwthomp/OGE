#include "framebuffer_object.h"

#include "glew/glew.h"

// Assumes that GLuint is same size as uint32.. This may need to change

framebuffer_object::framebuffer_object()
{
	m_width = 0;
	m_height = 0;
}

framebuffer_object::~framebuffer_object()
{
	if (m_depth_buffer_index) {
#if ORIG_DEPTH_BUFFER
		glDeleteRenderbuffersEXT(1, (GLuint const*)&m_depth_buffer_index);
#else
		glDeleteTextures(1, (GLuint *)&m_depth_buffer_index);
#endif
	}

	if (m_frame_buffer_index) {
		glDeleteFramebuffersEXT(1, (GLuint const*)&m_frame_buffer_index);
	}
}

void framebuffer_object::init(uint32 p_width, uint32 p_height, framebuffer_format p_format)
{
	m_width = p_width;
	m_height = p_height;

	glGenFramebuffersEXT(1, (GLuint *)&m_frame_buffer_index);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_frame_buffer_index);



	GLuint depth = (p_format & FRAMEBUFFER_FORMAT_DEPTH16) ? GL_DEPTH_COMPONENT16 :
						(p_format & FRAMEBUFFER_FORMAT_DEPTH24) ? GL_DEPTH_COMPONENT24 :
						(p_format & FRAMEBUFFER_FORMAT_DEPTH32) ? GL_DEPTH_COMPONENT32 : 0;

#if ORIG_DEPTH_BUFFER
	if (depth) {
		glGenRenderbuffersEXT(1, (GLuint *)&m_depth_buffer_index);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depth_buffer_index);
		glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, depth, p_width, p_height);
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depth_buffer_index);
	}
#else
	if (depth) {
		glGenTextures(1, (GLuint *)&m_depth_buffer_index);
		glBindTexture(GL_TEXTURE_2D, m_depth_buffer_index);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, p_width, p_height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_depth_buffer_index, 0);
	}
#endif

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

bool framebuffer_object::is_status_ready() const
{
	GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	
	if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
		return true;
	}

	return false;
}

void framebuffer_object::set_target_texture(uint32 p_texture_handle, uint8 p_color_buffer)
{
	if (m_frame_buffer_index) {
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + p_color_buffer, GL_TEXTURE_2D, p_texture_handle, 0);
	}
}

void framebuffer_object::bind() const
{
	if (m_frame_buffer_index) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_frame_buffer_index);
	}
}

void framebuffer_object::unbind() const
{
	if (m_frame_buffer_index) {
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
}

uint32 framebuffer_object::get_width() const
{
	return m_width;
}

uint32 framebuffer_object::get_height() const
{
	return m_height;
}

uint32 framebuffer_object::get_depth_buffer_id() const
{
	return m_depth_buffer_index;
}

void framebuffer_object_system_init()
{
	//glEnable(GL_TEXTURE_RECTANGLE_ARB);
	glEnable(GL_TEXTURE_2D);
}

void framebuffer_object_system_shutdown()
{
	// Free all framebuffer_objects?
}

framebuffer_object * framebuffer_object_create()
{
	return new framebuffer_object;
}

void framebuffer_object_destroy(framebuffer_object *p_framebuffer_object)
{
	delete p_framebuffer_object;
}