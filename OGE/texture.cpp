#include "texture.h"

#include "assert.h"
#include "allocator_array.h"
#include "ref_counted.h"

#include "glew/glew.h"

#include "SDL.h"
#include "SDL_image.h"

#include <stdio.h>
#include <string.h>

#ifdef MAC_OS_X
#define TEXTURE_PATH "OGE-osx.app/Contents/Resources/Data"
#else
#define TEXTURE_PATH "Data"
#endif


#define TEXTURE_MAX_NUMBER (256)

static ref_counted<texture, TEXTURE_MAX_NUMBER, allocator_array<ref_count_store<texture, TEXTURE_MAX_NUMBER>>> g_allocator_texture;


texture::texture()
{
	m_texture_data = NULL;
	m_texture_id = 0;
}

texture::~texture()
{
	if (m_texture_data != NULL) {
		free(m_texture_data);
		m_texture_data = NULL;
	}
}

void texture::bind(uint32 p_width, uint32 p_height)
{
	GLuint id;

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
		
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		
	//glTexImage2D(GL_TEXTURE_2D, 0, 3, p_w, p_h, 0, GL_BGR, GL_UNSIGNED_BYTE, p_pixel_data);
	
	// build our texture mipmaps
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, p_width, p_height, GL_RGB, GL_UNSIGNED_BYTE, m_texture_data);

	m_texture_id = id;
}

void texture::unbind()
{
	glDeleteTextures( 1, (GLuint *)&m_texture_id );
	m_texture_id = 0;
}

void texture::activate()
{
	glBindTexture(GL_TEXTURE_2D, m_texture_id);
}

void texture::load(char *p_texture_name)
{
	// Append data path
	char filename[4096];
	sprintf(filename, "%s/%s", TEXTURE_PATH, p_texture_name);

	// Load in a texture to opengl
	SDL_Surface *surf = IMG_Load(filename);
	assert(surf != NULL);
	if (surf == NULL) {
		return;
	}

	unsigned long pixel_size = surf->format->BytesPerPixel * surf->w * surf->h;
	m_texture_data = (uint8 *)malloc(pixel_size);
	memcpy(m_texture_data, surf->pixels, pixel_size);

	bind(surf->w, surf->h);

	SDL_FreeSurface(surf);
}

void texture_system_init()
{
	g_allocator_texture.init(TEXTURE_MAX_NUMBER);
}

void texture_system_shutdown()
{
	g_allocator_texture.shutdown();
}

texture * texture_create(char *p_name)
{
	return g_allocator_texture.create(p_name);
}

void texture_release(texture *p_texture)
{
	g_allocator_texture.release(p_texture);
}
