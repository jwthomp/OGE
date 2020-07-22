#include "render_block.h"

#include "render_lib.h"
#include "glew/glew.h"

render_block::render_block()
{
	m_prepared = false;
}

void render_block::prepare()
{
	m_display_list_id = glGenLists(1);
	glNewList(m_display_list_id, GL_COMPILE);

	render_lib_render_block(this, 0);

	glEndList();

	m_prepared = true;
}

void render_block::draw()
{
	glCallList(m_display_list_id);
}