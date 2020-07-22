#include "shader.h"

#include "allocator_array.h"
#include "ref_counted.h"
#include "assert.h"

#include "glew/glew.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef MAC_OS_X
#define SHADER_PATH "OGE-osx.app/Contents/Resources/"
#else
#define SHADER_PATH "Shader"
#endif

#define SHADER_EXTENSION_VERTEX ".shv"
#define SHADER_EXTENSION_FRAGMENT ".shf"

#define SHADER_MAX_NUMBER (32)

static ref_counted<shader, SHADER_MAX_NUMBER, allocator_array<ref_count_store<shader, SHADER_MAX_NUMBER>>> g_allocator_shader;


static void printShaderInfoLog(GLuint p_shader)
{
	GLint infologLen = 0;
	GLint charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(p_shader, GL_INFO_LOG_LENGTH, &infologLen);
	if (infologLen > 0) {
		infoLog = (GLchar *)malloc(infologLen);
		if (infoLog == NULL) {
			exit(1);
		}

		glGetShaderInfoLog(p_shader, infologLen, &charsWritten, infoLog);
		printf("InfoLog:\n%s\n\n", infoLog);
		assert(infologLen == 1);
		free(infoLog);
	}
}

static GLint getUniLoc(GLuint p_program, const GLchar *p_name)
{
	GLint loc = glGetUniformLocation(p_program, p_name);
	return loc;
}

shader::shader()
{
	m_shader_id = 0;
	m_fragment_id = 0;
	m_vertex_id = 0;
	m_loaded = false;
}

shader::~shader()
{
	if (m_fragment_id) {
		glDeleteShader(m_fragment_id);
		m_fragment_id = 0;
	}

	if (m_vertex_id) {
		glDeleteShader(m_vertex_id);
		m_vertex_id = 0;
	}

	if (m_shader_id) {
		glDeleteProgram(m_shader_id);
		m_shader_id = 0;
	}

	m_loaded = false;
}

void shader::load(char const* p_filename)
{
	GLuint vert_shader_id = glCreateShaderObjectARB(GL_VERTEX_SHADER);
	GLuint frag_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
	char vert_buffer[4096];
	char frag_buffer[4096];
	char buffer[256];
	size_t unit_size = 1;
	size_t unit_count = 4096;

	// Read in vertex shader
	sprintf(buffer, "%s/%s%s", SHADER_PATH, p_filename, SHADER_EXTENSION_VERTEX);
	FILE *fp = fopen(buffer, "r");	
	int lenv = fread(vert_buffer, unit_size, unit_count, fp);
	fclose(fp);

	// Read in fragment shader
	sprintf(buffer, "%s/%s%s", SHADER_PATH, p_filename, SHADER_EXTENSION_FRAGMENT);
	fopen_s(&fp, buffer, "r");
	int lenf = fread(frag_buffer, 1, 4096, fp);
	fclose(fp);

	// Set source
	char *vb = vert_buffer;
	char *vf = frag_buffer;
	glShaderSource(vert_shader_id, 1, const_cast<const GLchar**>(&vb), (GLint const *)&lenv);
	glShaderSource(frag_shader_id, 1, const_cast<const GLchar**>(&vf), (GLint const *)&lenf);

	glCompileShader(vert_shader_id);
	glCompileShader(frag_shader_id);

	GLint v_compiled, f_compiled;
	glGetShaderiv(vert_shader_id, GL_COMPILE_STATUS, &v_compiled);
	printShaderInfoLog(vert_shader_id);

	glGetShaderiv(frag_shader_id, GL_COMPILE_STATUS, &f_compiled);
	printShaderInfoLog(frag_shader_id);

	if (!v_compiled || !f_compiled) {
		glDeleteShader(m_fragment_id);
		glDeleteShader(m_vertex_id);
		return;
	}

	GLuint shader_prog = glCreateProgram();
	glAttachShader(shader_prog, vert_shader_id);
	glAttachShader(shader_prog, frag_shader_id);
	glLinkProgram(shader_prog);
	GLint s_linked;
	glGetProgramiv(shader_prog, GL_LINK_STATUS, &s_linked);

	if (!s_linked) {
		glDeleteShader(m_fragment_id);
		glDeleteShader(m_vertex_id);
		glDeleteProgram(shader_prog);
		return;
	}

#if 0
	glUniform3f(getUniLoc(shader_prog, "Tangent"), 1.0f, 0.0f, 0.0f);
	glUniform3f(getUniLoc(shader_prog, "LightPosition"), 0.0f, 1000.0f, 0.0f);
	glUniform3f(getUniLoc(shader_prog, "SurfaceColor"), 0.7f, 0.6f, 0.18f);
	glUniform1f(getUniLoc(shader_prog, "BumpDensity"), 16.0f);
	glUniform1f(getUniLoc(shader_prog, "BumpSize"), 0.15f);
	glUniform1f(getUniLoc(shader_prog, "SpecularFactor"), 0.5f);
#endif

	m_shader_id = shader_prog;
	m_vertex_id = vert_shader_id;
	m_fragment_id = frag_shader_id;
}

void shader::activate()
{
	glUseProgram(m_shader_id);
}

uint32 shader::get_location(char const* p_location_name)
{
	return glGetUniformLocationARB(m_shader_id, p_location_name);
}

void shader_system_init()
{
	g_allocator_shader.init(SHADER_MAX_NUMBER);
}

void shader_system_shutdown()
{
	g_allocator_shader.shutdown();
}

shader *shader_create(char *p_shader_name)
{
	shader *shader_ptr = g_allocator_shader.create(p_shader_name);

	if (shader_ptr && (shader_ptr->m_loaded == false)) {
		shader_ptr->load(p_shader_name);
		shader_ptr->m_loaded = true;
	}

	return shader_ptr;
}

void shader_release(shader *p_shader)
{
	g_allocator_shader.release(p_shader);
}

void shader_enable_fixed_function_pipeline()
{
	glUseProgram(0);
}
