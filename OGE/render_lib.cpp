#include "render_lib.h"

#include "mesh.h"
#include "mesh_instance_dynamic.h"
#include "mesh_instance.h"

#include "matrix.h"

#include "SDL.h"
#include "glew/glew.h"
#include "assert.h"
#include "framebuffer_object.h"

#include "shader.h"
#include "light.h"
#include "camera.h"
#include "matrix.h"

#include "frametime.h"

#include <list>
#include <map>


#include <windows.h>

#define DEFAULT_FOV (45.0f)
#define DEFAULT_CLIP_PLANE_NEAR (25.0f)
#define DEFAULT_CLIP_PLANE_FAR (32000.0f)
#define DEFAULT_WIDTH (640)
#define DEFAULT_HEIGHT (480)

#ifdef MAC_OS_X
#define BITMAP_NAME "OGE-osx.app/Contents/Resources/Tim.bmp"
#else
#define BITMAP_NAME "Tim.bmp"
#endif

bool g_swap = false;



//#define DEFER 1

static std::map<render_block *, std::list<mesh_instance *>>g_render_block_to_mesh_instance_list_map;
static std::map<shader *, std::map<render_block *, std::list<mesh_instance *>>>g_shader_to_render_blocks_map;

static std::list<mesh_instance *>g_mesh_instances;


static Vector3 g_camera_pos;
static quaternion g_camera_orient;

//static float g_camera_rots[3];
static GLuint g_texture[1];							// Storage For One Texture ( NEW )

static unsigned long g_width = 0;
static unsigned long g_height = 0;

static GLuint g_base_pass_color_texture;
static GLuint g_base_pass_normal_texture;
static GLuint g_base_pass_depth_texture;
static GLuint g_base_pass_posxy_texture;
static framebuffer_object g_framebuffer_object_base_pass;

static GLuint g_lighting_pass_color_texture;
static framebuffer_object g_framebuffer_object_lighting_pass;

static GLuint g_shadowmap_pass_depthBuffer;
static framebuffer_object g_framebuffer_object_shadowmap_pass;

static char g_shader_name[MAX_SHADER_NAME_LENGTH];
shader *g_shader_lighting;
static shader *g_shader_prepass;

static texture *g_texture_shadow;
static texture *g_texture_invalid;

camera g_camera;

static void add_render_block_to_shader(mesh_instance *p_mesh_instance, render_block *p_render_block)
{
	// 
	// First add in the render block to the shader list
	//
	assert(p_render_block != NULL);
	assert(p_render_block->m_material != NULL);

	shader *shader_ptr = p_render_block->m_material->m_shader;
	assert(shader_ptr != NULL);

	(g_shader_to_render_blocks_map[p_render_block->m_material->m_shader])[p_render_block].push_back(p_mesh_instance);
}

static SDL_Surface *LoadBMP(char *p_filename)
{
	return SDL_LoadBMP(p_filename);
}

static bool LoadGLTextures() {
	bool status = false;

	SDL_Surface *texture_image;

	if (texture_image = LoadBMP(BITMAP_NAME)) {
		status = true;

		glGenTextures(1, &g_texture[0]);
		glBindTexture(GL_TEXTURE_2D, g_texture[0]);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glTexImage2D(GL_TEXTURE_2D, 0, 3, texture_image->w, texture_image->h, 0, GL_BGR, GL_UNSIGNED_BYTE, texture_image->pixels);

		
	}

	if (texture_image) {
		SDL_FreeSurface(texture_image);
	}

	return status;
}

static GLvoid ReSizeGLScene(GLsizei p_width, GLsizei p_height)				// Resize And Initialize The GL Window
{
	// If height is 0, make it 1 to prevent divide by zero error
	if (p_height == 0) {
		p_height = 1;
	}

	// Reset The Current Viewport
	glViewport(0, 0, p_width, p_height);	

	// Select The Projection Matrix
	glMatrixMode(GL_PROJECTION);	

	// Reset The Projection Matrix
	glLoadIdentity();							

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(DEFAULT_FOV, (GLfloat)p_width / (GLfloat)p_height, DEFAULT_CLIP_PLANE_NEAR, DEFAULT_CLIP_PLANE_FAR);

	// Select The Modelview Matrix
	glMatrixMode(GL_MODELVIEW);			

	// Reset The Modelview Matrix
	glLoadIdentity();							
}


static void render_lib_map_2_0_functions()
{
#ifdef MAC_OS_X
	// Remap unsupported OpenGL 2.0 core functions for GLSL to supported ARB extension counterparts:
    if (NULL == glCreateProgram) glCreateProgram = (GLuint (*)(void))glCreateProgramObjectARB;
    if (NULL == glCreateShader) glCreateShader = (GLuint (*)(GLenum))glCreateShaderObjectARB;
    if (NULL == glShaderSource) glShaderSource = (void (*)(GLuint, GLsizei, const GLchar **, const GLint*))glShaderSourceARB;
    if (NULL == glCompileShader) glCompileShader = (void (*)(GLuint))glCompileShaderARB;
    if (NULL == glAttachShader) glAttachShader = (void (*)(GLuint, GLuint))glAttachObjectARB;
    if (NULL == glLinkProgram) glLinkProgram = (void (*)(GLuint))glLinkProgramARB;
    if (NULL == glUseProgram) glUseProgram = (void (*)(GLuint))glUseProgramObjectARB;
    if (NULL == glGetAttribLocation) glGetAttribLocation = (GLint (*)(GLuint, const GLchar*))glGetAttribLocationARB;
    // if (NULL == glGetUniformLocation) glGetUniformLocation = (GLint (*)(GLint, const GLchar*)) glGetUniformLocationARB;
    if (NULL == glGetUniformLocation) glGetUniformLocation = (GLint (*)(GLint, const GLchar*))glGetUniformLocationARB;
    if (NULL == glUniform1f) glUniform1f = glUniform1fARB;
    if (NULL == glUniform2f) glUniform2f = glUniform2fARB;
    if (NULL == glUniform3f) glUniform3f = glUniform3fARB;
    if (NULL == glUniform4f) glUniform4f = glUniform4fARB;
    if (NULL == glUniform1fv) glUniform1fv = glUniform1fvARB;
    if (NULL == glUniform2fv) glUniform2fv = glUniform2fvARB;
    if (NULL == glUniform3fv) glUniform3fv = glUniform3fvARB;
    if (NULL == glUniform4fv) glUniform4fv = glUniform4fvARB;
    if (NULL == glUniform1i) glUniform1i = glUniform1iARB;
    if (NULL == glUniform2i) glUniform2i = glUniform2iARB;
    if (NULL == glUniform3i) glUniform3i = glUniform3iARB;
    if (NULL == glUniform4i) glUniform4i = glUniform4iARB;
    if (NULL == glUniform1iv) glUniform1iv = glUniform1ivARB;
    if (NULL == glUniform2iv) glUniform2iv = glUniform2ivARB;
    if (NULL == glUniform3iv) glUniform3iv = glUniform3ivARB;
    if (NULL == glUniform4iv) glUniform4iv = glUniform4ivARB;
    if (NULL == glUniformMatrix2fv) glUniformMatrix2fv = glUniformMatrix2fvARB;
    if (NULL == glUniformMatrix3fv) glUniformMatrix3fv = glUniformMatrix3fvARB;
    if (NULL == glUniformMatrix4fv) glUniformMatrix4fv = glUniformMatrix4fvARB;
    if (NULL == glGetShaderiv) glGetShaderiv = (void (*)(GLuint, GLenum, GLint*))glGetObjectParameterivARB;
    if (NULL == glGetProgramiv) glGetProgramiv = (void (*)(GLuint, GLenum, GLint*))glGetObjectParameterivARB;
    if (NULL == glGetShaderInfoLog) glGetShaderInfoLog = (void (*)(GLuint, GLsizei, GLsizei*, GLchar*))glGetInfoLogARB;
    if (NULL == glGetProgramInfoLog) glGetProgramInfoLog = (void (*)(GLuint, GLsizei, GLsizei*, GLchar*))glGetInfoLogARB;
    if (NULL == glValidateProgram) glValidateProgram = (void (*)(GLuint))glValidateProgramARB;
    
    // Misc other stuff to remap...
    if (NULL == glDrawRangeElements) glDrawRangeElements = glDrawRangeElementsEXT;
    return;
#else
	// Remap unsupported OpenGL 2.0 core functions for GLSL to supported ARB extension counterparts:
    if (NULL == glCreateProgram) glCreateProgram = glCreateProgramObjectARB;
    if (NULL == glCreateShader) glCreateShader = glCreateShaderObjectARB;
    if (NULL == glShaderSource) glShaderSource = glShaderSourceARB;
    if (NULL == glCompileShader) glCompileShader = glCompileShaderARB;
    if (NULL == glAttachShader) glAttachShader = glAttachObjectARB;
    if (NULL == glLinkProgram) glLinkProgram = glLinkProgramARB;
    if (NULL == glUseProgram) glUseProgram = glUseProgramObjectARB;
    if (NULL == glGetAttribLocation) glGetAttribLocation = glGetAttribLocationARB;
    // if (NULL == glGetUniformLocation) glGetUniformLocation = (GLint (*)(GLint, const GLchar*)) glGetUniformLocationARB;
    if (NULL == glGetUniformLocation) glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glGetUniformLocationARB;
    if (NULL == glUniform1f) glUniform1f = glUniform1fARB;
    if (NULL == glUniform2f) glUniform2f = glUniform2fARB;
    if (NULL == glUniform3f) glUniform3f = glUniform3fARB;
    if (NULL == glUniform4f) glUniform4f = glUniform4fARB;
    if (NULL == glUniform1fv) glUniform1fv = glUniform1fvARB;
    if (NULL == glUniform2fv) glUniform2fv = glUniform2fvARB;
    if (NULL == glUniform3fv) glUniform3fv = glUniform3fvARB;
    if (NULL == glUniform4fv) glUniform4fv = glUniform4fvARB;
    if (NULL == glUniform1i) glUniform1i = glUniform1iARB;
    if (NULL == glUniform2i) glUniform2i = glUniform2iARB;
    if (NULL == glUniform3i) glUniform3i = glUniform3iARB;
    if (NULL == glUniform4i) glUniform4i = glUniform4iARB;
    if (NULL == glUniform1iv) glUniform1iv = glUniform1ivARB;
    if (NULL == glUniform2iv) glUniform2iv = glUniform2ivARB;
    if (NULL == glUniform3iv) glUniform3iv = glUniform3ivARB;
    if (NULL == glUniform4iv) glUniform4iv = glUniform4ivARB;
    if (NULL == glUniformMatrix2fv) glUniformMatrix2fv = glUniformMatrix2fvARB;
    if (NULL == glUniformMatrix3fv) glUniformMatrix3fv = glUniformMatrix3fvARB;
    if (NULL == glUniformMatrix4fv) glUniformMatrix4fv = glUniformMatrix4fvARB;
    if (NULL == glGetShaderiv) glGetShaderiv = glGetObjectParameterivARB;
    if (NULL == glGetProgramiv) glGetProgramiv = glGetObjectParameterivARB;
    if (NULL == glGetShaderInfoLog) glGetShaderInfoLog = glGetInfoLogARB;
    if (NULL == glGetProgramInfoLog) glGetProgramInfoLog = glGetInfoLogARB;
    if (NULL == glValidateProgram) glValidateProgram = glValidateProgramARB;
    
    // Misc other stuff to remap...
    if (NULL == glDrawRangeElements) glDrawRangeElements = glDrawRangeElementsEXT;
    return;
#endif
}

static bool IsExtensionSupported( char* szTargetExtension )
{
	const unsigned char *pszExtensions = NULL;
	const unsigned char *pszStart;
	unsigned char *pszWhere, *pszTerminator;
	
	// Extension names should not have spaces
	pszWhere = (unsigned char *) strchr( szTargetExtension, ' ' );
	if( pszWhere || *szTargetExtension == '\0' )
		return false;
	
	// Get Extensions String
	pszExtensions = glGetString( GL_EXTENSIONS );
	
	// Search The Extensions String For An Exact Copy
	pszStart = pszExtensions;
	for(;;)
	{
		pszWhere = (unsigned char *) strstr( (const char *) pszStart, szTargetExtension );
		if( !pszWhere )
			break;
		pszTerminator = pszWhere + strlen( szTargetExtension );
		if( pszWhere == pszStart || *( pszWhere - 1 ) == ' ' )
			if( *pszTerminator == ' ' || *pszTerminator == '\0' )
				return true;
		pszStart = pszTerminator;
	}
	return false;
}

static void setup_base_pass_framebuffer()
{
	g_framebuffer_object_base_pass.init(g_width, g_height, framebuffer_object::FRAMEBUFFER_FORMAT_DEPTH24);

	// Bind a color texture to target
	glGenTextures(1, &g_base_pass_color_texture);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind a normal texture to target
	glGenTextures(1, &g_base_pass_normal_texture);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_normal_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind a normal texture to target
	glGenTextures(1, &g_base_pass_posxy_texture);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_posxy_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Bind a depth texture to target
	glGenTextures(1, &g_base_pass_depth_texture);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);


	g_framebuffer_object_base_pass.bind();
	g_framebuffer_object_base_pass.set_target_texture(g_base_pass_color_texture, 0);
	g_framebuffer_object_base_pass.set_target_texture(g_base_pass_normal_texture, 1);
	g_framebuffer_object_base_pass.set_target_texture(g_base_pass_depth_texture, 2);
	g_framebuffer_object_base_pass.set_target_texture(g_base_pass_posxy_texture, 3);

	bool ret = g_framebuffer_object_base_pass.is_status_ready();
	if (ret == false) {
		assert(!"framebuffer object not initialized successfully\n");
	}

	g_framebuffer_object_base_pass.unbind();


}

static void setup_lighting_pass_framebuffer()
{
	g_framebuffer_object_lighting_pass.init(g_width, g_height, framebuffer_object::FRAMEBUFFER_FORMAT_DEPTH24);

	// Bind a color texture to target
	glGenTextures(1, &g_lighting_pass_color_texture);
	glBindTexture(GL_TEXTURE_2D, g_lighting_pass_color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  g_width, g_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	g_framebuffer_object_lighting_pass.bind();
	g_framebuffer_object_lighting_pass.set_target_texture(g_lighting_pass_color_texture, 0);
	g_framebuffer_object_lighting_pass.unbind();

	bool ret = g_framebuffer_object_lighting_pass.is_status_ready();
	if (ret == false) {
		assert(!"framebuffer object not initialized successfully\n");
	}
}

static void setup_shadowmap_pass_framebuffer()
{
	g_framebuffer_object_shadowmap_pass.init(g_width, g_height, framebuffer_object::FRAMEBUFFER_FORMAT_DEPTH24);

	bool ret = g_framebuffer_object_lighting_pass.is_status_ready();
	if (ret == false) {
		assert(!"framebuffer object not initialized successfully\n");
	}
}

static void draw_geometry(bool p_depthonly, matrix44 const* modelview_mat)
{
	if (p_depthonly) {
		g_shader_prepass->activate();
	}

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);


	glActiveTexture(GL_TEXTURE0);

	

	bool texture_coord = false;
	bool normal = false;
	shader *active_shader = 0;
	uint32 my_sampler_uniform_location;

	uint32 triangle_count = 0;
	uint32 block_count = 0;

	uint32 shader_count = 0;

//#define RENDER_LIST
#if defined(RENDER_LIST)
	std::list<mesh_instance *>::iterator mesh_iter;
	for (mesh_iter = g_mesh_instances.begin(); mesh_iter != g_mesh_instances.end(); ++mesh_iter) {
		mesh const* mesh_ptr = (*mesh_iter)->m_mesh;

		for (unsigned long i = 0; i < mesh_ptr->m_render_block_count; i++) {
			render_block &rb = mesh_ptr->m_render_blocks[i];

			glPushMatrix();

			mesh_instance *mi = *mesh_iter;
			glTranslatef(mi->m_position.m_data[0], mi->m_position.m_data[1], mi->m_position.m_data[2]);
			matrix44 matrix;
			mi->m_orientation.CreateMatrix(matrix.m_data);
			glMultMatrixf(matrix.m_data);
			
			glScalef(mi->m_scale.m_data[0], mi->m_scale.m_data[1], mi->m_scale.m_data[2]);

			//glMultMatrixf(rb.m_transform.m_data);

			if (p_depthonly == false) {
				assert(rb.m_material->m_shader != NULL);
				if (rb.m_material->m_shader && (active_shader != rb.m_material->m_shader)) {
					rb.m_material->m_shader->activate();
					my_sampler_uniform_location = rb.m_material->m_shader->get_location("base_texture");
					glUniform1iARB(my_sampler_uniform_location, 0);
					active_shader = rb.m_material->m_shader;

					uint32 uniform_location8 = rb.m_material->m_shader->get_location("camera_position");
					uint32 uniform_location9 = rb.m_material->m_shader->get_location("camera_direction");
					uint32 uniform_location10 = rb.m_material->m_shader->get_location("depth_near");
					uint32 uniform_location11 = rb.m_material->m_shader->get_location("depth_far");

					matrix44 orient;
					g_camera_orient.CreateMatrix(orient.m_data);
	
					glUniform3fvARB(uniform_location8, 1, g_camera_pos.m_data);
					glUniform3fvARB(uniform_location9, 1, orient.get_fvec().m_data);
					glUniform1fARB(uniform_location10, DEFAULT_CLIP_PLANE_NEAR);
					glUniform1fARB(uniform_location11, DEFAULT_CLIP_PLANE_FAR);
				}
			}

			if (rb.m_prepared == false) {
				rb.prepare();
			}

#if 0
	matrix44 orient;
	g_camera_orient.CreateMatrix(orient.m_data);
	glMultMatrixf(orient.m_data);
	glTranslatef( g_camera_pos.m_data[0], g_camera_pos.m_data[1], g_camera_pos.m_data[2]);
#endif


			if (rb.m_prepared == true) {
				rb.draw();
			} else {
				if ((*mesh_iter)->m_type == RENDER_LIB_MESH_INSTANCE_TYPE_DYNAMIC) {
					mesh_instance_dynamic *mid = (mesh_instance_dynamic *)*mesh_iter;
					render_lib_render_block(&rb, mid);
				} else {
					render_lib_render_block(&rb, 0);
				}
			}

			triangle_count += rb.m_index_count / 3;
			block_count++;

			

			glPopMatrix();
		}
	}
#else /* RENDER_LIST */
	//static std::map<shader *, std::map<render_block *, std::list<mesh_instance *>>>g_shader_to_render_blocks_map;

	std::map<shader *, std::map<render_block *, std::list<mesh_instance *>>>::iterator shader_iter;
	std::map<render_block *, std::list<mesh_instance *>>::iterator render_block_iter;
	std::list<mesh_instance *>::iterator mesh_instance_iter;
	// For each shader
	for (shader_iter = g_shader_to_render_blocks_map.begin(); shader_iter != g_shader_to_render_blocks_map.end(); ++shader_iter) {
		// For each render block
		shader *shader_ptr = (*shader_iter).first;
		shader_count++;

		if (p_depthonly == false) {
			shader_ptr->activate();

			my_sampler_uniform_location = shader_ptr->get_location("base_texture");
			glUniform1iARB(my_sampler_uniform_location, 0);

			uint32 uniform_location3 = shader_ptr->get_location("light_position");
			light *light_ptr = light_get_from_index(0);
			Vector3 const& pos = light_ptr->m_transform.get_position();
			Vector3 light_pos = *modelview_mat * pos;
			glUniform4fARB(uniform_location3, light_pos.m_data[0], light_pos.m_data[1], light_pos.m_data[2], 1.0f);

			uint32 uniform_location6 = shader_ptr->get_location("light_diffuse");
			glUniform4fvARB(uniform_location6, 1, light_ptr->m_diffuse);

			uint32 uniform_location5 = shader_ptr->get_location("light_ambient");
			glUniform4fvARB(uniform_location5, 1, light_ptr->m_ambient);

		}

		for (render_block_iter = (*shader_iter).second.begin(); render_block_iter != (*shader_iter).second.end(); ++render_block_iter) {
			// For each mesh instance
			render_block *render_block_ptr = (*render_block_iter).first;

			render_block &rb = *render_block_ptr;

			if (rb.m_uv != NULL) {
				glTexCoordPointer(2, GL_FLOAT, 0, rb.m_uv);
			}
			
			if (rb.m_normal) {
				glNormalPointer(GL_FLOAT, 0, rb.m_normal);
			}
			
			if (rb.m_material) {
#if MATERIAL_SUPPORT
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rb.m_material->m_color_ambient);
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rb.m_material->m_color_diffuse);
				glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, rb.m_material->m_color_spec);
#endif
							
				if (rb.m_material->m_texture != 0) {
					if (rb.m_material->m_shader != NULL) {
						rb.m_material->m_texture->activate();
					} else {
						rb.m_material->m_texture->activate();
					}
				}
				
			} else {
				//glColor3f(mi->m_color.data[0], mi->m_color.data[1], mi->m_color.data[2]);
				glColor3f(0.0f, 0.0f, 0.0f);
			}

			block_count++;

			for (mesh_instance_iter = (*render_block_iter).second.begin(); mesh_instance_iter != (*render_block_iter).second.end(); ++mesh_instance_iter) {
				glPushMatrix();

				mesh_instance *mi = *mesh_instance_iter;

				glMultMatrixf(mi->m_transform.m_transform_matrix.m_data);

				if (rb.m_prepared == false) {
					rb.m_display_list_id = glGenLists(1);
					glNewList(rb.m_display_list_id, GL_COMPILE);
					glVertexPointer(3, GL_FLOAT, 0, rb.m_pos);
					switch (rb.m_format) {
						case RENDER_LIB_MESH_FORMAT_VA_TRIANGLES:
							glDrawElements(GL_TRIANGLES, rb.m_index_count, GL_UNSIGNED_INT, rb.m_index_buffer);
							break;
						case RENDER_LIB_MESH_FORMAT_VA_TRIANGLE_STRIP:
							glDrawElements(GL_TRIANGLE_STRIP, rb.m_index_count, GL_UNSIGNED_INT, rb.m_index_buffer);
							break;
						default:
							break;
					};
					glEndList();

					rb.m_prepared = true;
				}

				glCallList(rb.m_display_list_id);

				triangle_count += render_block_ptr->m_index_count / 3;

				glPopMatrix();
			}
		}
	}


#endif /* RENDER_LIST */

	{
				
		static int count = 0;
		if (count >= 100) {
			char buffer[256];
			sprintf(buffer, "blocks: %u tri's: %u  shaders: %u\n", block_count, triangle_count, shader_count);
			OutputDebugStringA(buffer);
			count = 0;
		}
		count++;
	}

}

static void draw_final_scene()
{
	// Step Four: Render final texture to scene
	glActiveTexture(GL_TEXTURE0);

	if (g_swap) {
		glBindTexture(GL_TEXTURE_2D, g_base_pass_posxy_texture); // g_lighting_pass_color_texture); //g_lighting_pass_color_texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, g_lighting_pass_color_texture); // g_lighting_pass_color_texture); //g_lighting_pass_color_texture);
	}

	shader_enable_fixed_function_pipeline();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

	// Select The Projection Matrix
	glMatrixMode(GL_PROJECTION);	

	// Reset The Projection Matrix
	glLoadIdentity();							

	// Calculate The Aspect Ratio Of The Window
	glOrtho(0, g_width, 0, g_height, -1.0, 1.0);

	// Select The Modelview Matrix
	glMatrixMode(GL_MODELVIEW);			

	// Reset The Modelview Matrix
	glLoadIdentity();


	//glTranslatef(-250.0f, -250.0f, 0.0f);

	//LoadGLTextures();
	//glEnable(GL_TEXTURE_2D);
	//glBindTexture(GL_TEXTURE_2D, g_texture[0]);

	// Draw quad
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); 
	glVertex2i(0, g_height);	// Bottom Left Of The Texture and Quad

	glTexCoord2i(1, 0);
	glVertex2i(g_width, g_height);	// Bottom Right Of The Texture and Quad

	glTexCoord2i(1, 1);
	glVertex2i(g_width, 0);	// Top Right Of The Texture and Quad

	glTexCoord2i(0, 1);
	glVertex2i(0, 0);	// Top Left Of The Texture and Quad
	glEnd();

	ReSizeGLScene(g_width, g_height);
}


static void generate_view_vectors(Vector3 p_view_vectors[4], uint32 p_width, uint32 p_height, matrix44 const* p_view_mat, matrix44 const * p_proj_mat, Vector3& p_camera_pos)
{
	int pixels[4][2] = {{0,0}, {0, p_height}, {p_width, p_height}, {p_width, 0}};
	int viewport[4] = {0, 0, p_width, p_height};

	matrix44 view_rotation = *p_view_mat;
	view_rotation.set_translation(Vector3(0.0f, 0.0f, 0.0f));

	real64 obj_pos[3];

	real64 view_mat_data[16];
	real64 proj_mat_data[16];
	for (uint32 i = 0; i < 16; i++) {
		view_mat_data[i] = p_view_mat->m_data[i];
		proj_mat_data[i] = p_proj_mat->m_data[i];
	}

	for (int i = 0; i < 4; i++) {
		gluUnProject(pixels[i][0], pixels[i][1], 10, view_mat_data, proj_mat_data, viewport, &obj_pos[0], &obj_pos[1], &obj_pos[2]);
		p_view_vectors[i].set((real)obj_pos[0], (real)obj_pos[1], (real)obj_pos[2]);
		p_view_vectors[i] -= p_camera_pos;
		p_view_vectors[i] = p_view_vectors[i] / p_view_vectors[i].len();
		p_view_vectors[i] = view_rotation * p_view_vectors[i];
	}
}

static void draw_lights(matrix44 const *modelview_mat, matrix44 const* view_proj_inv, matrix44 const *proj_mat_inv)
{
#define RENDER_LIGHTS
#if defined (RENDER_LIGHTS)

	glPushAttrib(GL_CURRENT_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_EVAL_BIT);

	// Bind framebuffer object
	g_framebuffer_object_lighting_pass.bind();
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glClear(GL_COLOR_BUFFER_BIT);

	// Set shader
	g_shader_lighting->activate();

	uint32 uniform_location0 = g_shader_lighting->get_location("color_texture");
	uint32 uniform_location1 = g_shader_lighting->get_location("normal_texture");
	uint32 uniform_location4 = g_shader_lighting->get_location("depth_texture");

	uint32 uniform_location3 = g_shader_lighting->get_location("light_position");
	uint32 uniform_location5 = g_shader_lighting->get_location("light_ambient");
	uint32 uniform_location6 = g_shader_lighting->get_location("light_diffuse");
	uint32 uniform_location7 = g_shader_lighting->get_location("light_specular");
	uint32 uniform_location8 = g_shader_lighting->get_location("camera_position");
	uint32 uniform_location9 = g_shader_lighting->get_location("camera_direction");

	//uint32 uniform_location12 = g_shader_lighting->get_location("shadow_texture");
	//uint32 uniform_location13 = g_shader_lighting->get_location("shadow_map_texture");
	uint32 uniform_location14 = g_shader_lighting->get_location("light_attenuation");
	//uint32 uniform_location15 = g_shader_lighting->get_location("view_matrix");
	//uint32 uniform_location16 = g_shader_lighting->get_location("view_matrix_inverse");
	uint32 uniform_location17 = g_shader_lighting->get_location("spot_direction");
	uint32 uniform_location18 = g_shader_lighting->get_location("spot_exponent");
	uint32 uniform_location19 = g_shader_lighting->get_location("spot_cos_cutoff");
	uint32 uniform_location20 = g_shader_lighting->get_location("posxy_texture");
	//uint32 uniform_location21 = g_shader_lighting->get_location("view_vectors");
	//uint32 uniform_location22 = g_shader_lighting->get_location("planes");
	uint32 uniform_location23 = g_shader_lighting->get_location("proj_matrix_inverse");
	uint32 uniform_location24 = g_shader_lighting->get_location("light_specular_power");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_color_texture);
	glUniform1iARB(uniform_location0, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_normal_texture);
	glUniform1iARB(uniform_location1, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_depth_texture);
	glUniform1iARB(uniform_location4, 2);

	//glActiveTexture(GL_TEXTURE3);
	//g_texture_shadow->activate();
	//glUniform1iARB(uniform_location12, 3);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, g_base_pass_posxy_texture);
	glUniform1iARB(uniform_location20, 4);

	//glActiveTexture(GL_TEXTURE5);
	//glBindTexture(GL_TEXTURE_2D, g_framebuffer_object_shadowmap_pass.get_depth_buffer_id());
	//glUniform1iARB(uniform_location13, 5);

	glUniform3fvARB(uniform_location8, 1, g_camera_pos.m_data);
	matrix44 orient;
	g_camera_orient.CreateMatrix(orient.m_data);

	glUniform3fvARB(uniform_location9, 1, orient.get_fvec().m_data);

	//glUniformMatrix4fvARB(uniform_location15, 1, 0, modelview_mat->m_data);

	glUniformMatrix4fvARB(uniform_location23, 1, 0, proj_mat_inv->m_data);
	
	//glUniform3fARB(uniform_location3, 0.0f, -25.0f, -500.0f);
	
	// Call shader for quad
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

	// Select The Projection Matrix
	glMatrixMode(GL_PROJECTION);	

	// Reset The Projection Matrix
	glLoadIdentity();							

	// Calculate The Aspect Ratio Of The Window
	glOrtho(0, g_width, 0, g_height, 0.0, 1.0);

	// Select The Modelview Matrix
	glMatrixMode(GL_MODELVIEW);			

	// Reset The Modelview Matrix
	glLoadIdentity();

	//glDepthMask(false);
	//glColorMask(true, true, true, true);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	//glDepthFunc(GL_EQUAL);

	//g_framebuffer_object_lighting_pass.unbind();

	//g_framebuffer_object_lighting_pass.bind();
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glClear(GL_COLOR_BUFFER_BIT);
	//g_shader_lighting->activate();

	
	bool setup = false;

	// Loop over lights
	for (uint32 i = 0; i < light_get_count(); ++i) {
		light *light_ptr = light_get_from_index((uint8)i);

		if (light_ptr->m_type == light::LIGHT_TYPE_NONE) {
			continue;
		}

		if (light_ptr->m_shadow_casting) {
			g_framebuffer_object_lighting_pass.unbind();

			// Render from light's perspective and get depth map
			// Enable Shadow FBO
			// Transform to light
			g_framebuffer_object_shadowmap_pass.bind();

			ReSizeGLScene(g_width, g_height);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			g_shader_prepass->activate();
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glEnable(GL_DEPTH_TEST);
			glCullFace(GL_FRONT);
			glDepthMask(GL_TRUE);
			glDrawBuffer(GL_FALSE);
			glReadBuffer (GL_FALSE);

			// Transform to lights position
			camera light_camera;
			light_camera.set_transform(&light_ptr->m_transform.m_transform_matrix);
			light_camera.set_perspective();

			draw_geometry(true, NULL);

			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			g_framebuffer_object_shadowmap_pass.unbind();

			glCullFace(GL_BACK);

			g_framebuffer_object_lighting_pass.bind();
			g_shader_lighting->activate();
		}


		// Render
		// Disable Shadow FBO
		// Render front face culled shape of light and pass in shadow depth buffer
		
		// Set light values
		Vector3 const& pos = light_ptr->m_transform.get_position();
		if (light_ptr->m_type == light::LIGHT_TYPE_POINT) {
			Vector3 light_pos = *modelview_mat * pos;
			glUniform4fARB(uniform_location3, light_pos.m_data[0], light_pos.m_data[1], light_pos.m_data[2], 1.0f);
		} else {	// DIRECTION LIGHT
			Vector3 light_pos = *modelview_mat * pos;
			glUniform4fARB(uniform_location3, light_pos.m_data[0], light_pos.m_data[1], light_pos.m_data[2], 0.0f);
		}
		glUniform4fvARB(uniform_location5, 1, light_ptr->m_ambient);
		glUniform4fvARB(uniform_location6, 1, light_ptr->m_diffuse);
		glUniform4fvARB(uniform_location7, 1, light_ptr->m_specular);
		float attenuation[3];
		attenuation[0] = light_ptr->m_constant_attenuation;
		attenuation[1] = light_ptr->m_linear_attenuation;
		attenuation[2] = light_ptr->m_quadratic_attenuation;
		glUniform3fvARB(uniform_location14, 1, attenuation);

		Vector3 spot_direction = (*modelview_mat * light_ptr->m_spot_direction) - (*modelview_mat * Vector3(0.0f, 0.0f, 0.0f));

		glUniform3fvARB(uniform_location17, 1, spot_direction.m_data);
		glUniform1fARB(uniform_location18, light_ptr->m_spot_exponent);
		glUniform1fARB(uniform_location19, light_ptr->m_spot_cos_cutoff);
		glUniform1fARB(uniform_location24, light_ptr->m_specular_power);

#if defined(DEBUG_RENDER)
		static int count = 0;
		if (count >= 100) {
			char buffer[256];
			sprintf(buffer, "light pos: %f %f %f att: %f %f %f  diff: %f %f %f %f\n", pos.m_data[0], pos.m_data[1], pos.m_data[2],
				attenuation[0], attenuation[1], attenuation[2], 
				light_ptr->m_diffuse[0], light_ptr->m_diffuse[1], light_ptr->m_diffuse[2], light_ptr->m_diffuse[3]);
			OutputDebugStringA(buffer);
			count = 0;
		}
		count++;
#endif

		// Select The Projection Matrix
		glMatrixMode(GL_PROJECTION);	

		// Reset The Projection Matrix
		glLoadIdentity();							

		// Calculate The Aspect Ratio Of The Window
		glOrtho(0, g_width, 0, g_height, -1.0, 1.0);

		// Select The Modelview Matrix
		glMatrixMode(GL_MODELVIEW);			

		// Reset The Modelview Matrix
		glLoadIdentity();

		//glDepthMask(false);
		//glColorMask(true, true, true, true);

		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//glDepthFunc(GL_EQUAL);

		//g_framebuffer_object_lighting_pass.unbind();

		//g_framebuffer_object_lighting_pass.bind();
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

		// Draw quad
		glBegin(GL_QUADS);
		glTexCoord2f( 0.0f, 1.0f); 
		glVertex2i(0, g_height);	// Bottom Left Of The Texture and Quad

		//glTexCoord2i(g_width, 0);
		glTexCoord2f(1.0f, 1.0f);
		glVertex2i(g_width, g_height);	// Bottom Right Of The Texture and Quad

		//glTexCoord2i(g_width, g_height);
		glTexCoord2f(1.0f, 0.0f);
		glVertex2i(g_width, 0);	// Top Right Of The Texture and Quad

		//glTexCoord2i(0, g_height);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2i(0, 0);	// Top Left Of The Texture and Quad
		glEnd();

		//g_framebuffer_object_lighting_pass.unbind();

		light_release(light_ptr);
	}

	//glDepthMask(true);
	//glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);

	g_framebuffer_object_lighting_pass.unbind();

	glPopAttrib();

	ReSizeGLScene(g_width, g_height);
#endif
}

void render_lib_render_block(render_block *p_render_block, mesh_instance_dynamic *p_dynamic_mesh)
{
	render_block &rb = *p_render_block;

	if (rb.m_uv != NULL) {
		glTexCoordPointer(2, GL_FLOAT, 0, rb.m_uv);
	}
	
	if (rb.m_normal) {
		glNormalPointer(GL_FLOAT, 0, rb.m_normal);
	}
	
	if (rb.m_material) {
#if MATERIAL_SUPPORT
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, rb.m_material->m_color_ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, rb.m_material->m_color_diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, rb.m_material->m_color_spec);
#endif
					
		

		if (rb.m_material->m_texture != 0) {
			if (rb.m_material->m_shader != NULL) {
				rb.m_material->m_texture->activate();
			} else {
				rb.m_material->m_texture->activate();
			}
		}
		
	} else {
		//glColor3f(mi->m_color.data[0], mi->m_color.data[1], mi->m_color.data[2]);
		glColor3f(0.0f, 0.0f, 0.0f);
	}

	if (p_dynamic_mesh == 0) { 
		glVertexPointer(3, GL_FLOAT, 0, rb.m_pos);
	} else {
		glVertexPointer(3, GL_FLOAT, 0, p_dynamic_mesh->m_dynamic_pos);
	}
	
	switch (rb.m_format) {
		case RENDER_LIB_MESH_FORMAT_VA_TRIANGLES:
			glDrawElements(GL_TRIANGLES, rb.m_index_count, GL_UNSIGNED_INT, rb.m_index_buffer);
			break;
		case RENDER_LIB_MESH_FORMAT_VA_TRIANGLE_STRIP:
			glDrawElements(GL_TRIANGLE_STRIP, rb.m_index_count, GL_UNSIGNED_INT, rb.m_index_buffer);
			break;
		default:
			break;
		};
}



void render_lib_set_default_shader(char *p_shader_name)
{
	strncpy(g_shader_name, p_shader_name, MAX_SHADER_NAME_LENGTH);
}

shader * render_lib_get_default_shader()
{
	return shader_create(g_shader_name);
}

bool render_lib_init(unsigned long p_width, unsigned long p_height)
{
	g_width = p_width;
	g_height = p_height;


	/* Information about the current video settings. */
	const SDL_VideoInfo* info = NULL;
	
	/* Color depth in bits of our window. */
	int bpp = 0;

	/* First, initialize SDL's video subsystem. */
	if( SDL_InitSubSystem( SDL_INIT_VIDEO) < 0 ) {
		/* Failed, exit. */
		fprintf( stderr, "Video initialization failed: %s\n",
					SDL_GetError( ) );
		SDL_Quit( );
		exit(1);
	}
	
	/* Let's get some video information. */
	info = SDL_GetVideoInfo( );
	
	if( !info ) {
		/* This should probably never happen. */
		fprintf( stderr, "Video query failed: %s\n",
					SDL_GetError( ) );
		SDL_Quit( );
		exit(1);
	}
	
	bpp = info->vfmt->BitsPerPixel;
	
	/*
	 * Now, we want to setup our requested
	 * window attributes for our OpenGL window.
	 * We want *at least* 5 bits of red, green
	 * and blue. We also want at least a 16-bit
	 * depth buffer.
	 *
	 * The last thing we do is request a double
	 * buffered window. '1' turns on double
	 * buffering, '0' turns it off.
	 *
	 * Note that we do not use SDL_DOUBLEBUF in
	 * the flags to SDL_SetVideoMode. That does
	 * not affect the GL attribute state, only
	 * the standard 2D blitting setup.
	 */
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	
	/* Flags we will pass into SDL_SetVideoMode. */
	int flags = 0;
	
	/*
	 * We want to request that SDL provide us
	 * with an OpenGL window, in a fullscreen
	 * video mode.
	 *
	 * EXERCISE:
	 * Make starting windowed an option, and
	 * handle the resize events properly with
	 * glViewport.
	 */
	flags = SDL_OPENGL; // | SDL_FULLSCREEN;
	
	/*
	 * Set the video mode
	 */
	if( SDL_SetVideoMode( p_width, p_height, bpp, flags ) == 0 ) {
		/* 
		* This could happen for a variety of reasons,
		 * including DISPLAY not being set, the specified
		 * resolution not being available, etc.
		 */
		fprintf( stderr, "Video mode set failed: %s\n",
					SDL_GetError( ) );
		SDL_Quit();
		exit(1);
	}
	
	bool val = IsExtensionSupported( "GL_ARB_vertex_buffer_object" );
	if (val == false) {
		printf("ERROR: Graphics Card does not support GL_ARB_vertex_buffer_object\n");
		return false;
	}

	val = IsExtensionSupported("GL_EXT_draw_instanced");
	if (val == false) {
//		assert(0 == 1);
	}
	
	GLenum err = glewInit();
	
	render_lib_map_2_0_functions();

	// Enables Smooth Shading
	glShadeModel(GL_SMOOTH);

	// Black Background
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);		

	// Depth Buffer Setup
	glClearDepth(1.0f);				

	// Enables Depth Testing
	glEnable(GL_DEPTH_TEST);	

	// The Type Of Depth Test To Do
	glDepthFunc(GL_LEQUAL);

	glClearDepth(1.0f);

	// Really Nice Perspective Calculations
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	

	glPolygonMode( GL_BACK, GL_FILL );			// Back Face Is Filled In
	glPolygonMode( GL_FRONT, GL_FILL );			// Front Face Is Drawn With Lines

	// setup fog
	//glFogfv(GL_FOG_COLOR, m_fvFogColor);
	//glFogf(GL_FOG_DENSITY, 0.0005f);

	// setup background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	ReSizeGLScene(p_width, p_height);

	material_system_init();
	texture_system_init();
	shader_system_init();
	framebuffer_object_system_init();
	light_system_init();

	setup_base_pass_framebuffer();
	setup_lighting_pass_framebuffer();
	setup_shadowmap_pass_framebuffer();

	g_shader_lighting = shader_create("deferred_lighting");
	g_shader_prepass = shader_create("prepass");

	g_texture_shadow = texture_create("shadow_blend_texture");
	g_texture_shadow->load("shadow.png");

	g_texture_invalid = texture_create("invalid_texture");
	g_texture_invalid->load("invalid.png");

	return true;								
}

mesh_id render_lib_mesh_instance_add(mesh_instance *p_mesh_instance)
{
	for (uint32 i = 0; i < p_mesh_instance->m_mesh->m_render_block_count; ++i) {
		add_render_block_to_shader(p_mesh_instance, &p_mesh_instance->m_mesh->m_render_blocks[i]);
	}


	g_mesh_instances.push_back(p_mesh_instance);
	return (mesh_id)0;
}

void render_lib_mesh_instance_remove(mesh_instance *p_mesh_instance)
{
#if 0
	// Remove mesh instance for render block list
	std::list<mesh_instance *>::iterator iter;

	for (uint32 i = 0; i < p_mesh_instance->m_mesh->m_render_block_count; ++i) {
		render_block *render_block_ptr = &p_mesh_instance->m_mesh->m_render_blocks[i];

		for (iter = g_render_block_to_mesh_instance_list_map[render_block_ptr].begin(); iter != g_render_block_to_mesh_instance_list_map[render_block_ptr].end(); ++iter) {
			if ((*iter) == p_mesh_instance) {
				g_render_block_to_mesh_instance_list_map[render_block_ptr].erase(iter);
				break;
			}
		}

		if (g_render_block_to_mesh_instance_list_map[render_block_ptr].empty() == true) {
			// If render block has no entries, remove it from shader list
			assert (render_block_ptr->m_material != NULL);
			assert (render_block_ptr->m_material->m_shader != NULL);
			shader *shader_ptr = render_block_ptr->m_material->m_shader;

			std::list<render_block *>::iterator iter2;

			for (iter2 = g_shader_to_render_blocks_map[shader_ptr].begin(); iter2 != g_shader_to_render_blocks_map[shader_ptr].end(); ++iter2) {
				if ((*iter2) == render_block_ptr) {
					g_shader_to_render_blocks_map[shader_ptr].erase(iter2);
					break;
				}
			}
		}
	}
#endif

	std::list<mesh_instance *>::iterator mesh_iter;
	for (mesh_iter = g_mesh_instances.begin(); mesh_iter != g_mesh_instances.end(); ++mesh_iter) {
		if (*mesh_iter == p_mesh_instance) {
			g_mesh_instances.erase(mesh_iter);
			return;
		}
	}
}

void render_lib_set_camera(Vector3 const& p_pos, quaternion const& p_orient)
{
	g_camera_pos = p_pos;
	g_camera_orient = p_orient;
}

void render_lib_render()
{
	// Step One: Render non-lit scene to texture
	g_framebuffer_object_base_pass.bind();
	GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT };

	glDrawBuffers(4, buffers);
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

	glDrawBuffer(GL_COLOR_ATTACHMENT1_EXT);
	//glClearColor(0.5f, 0.0f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);		

	glDrawBuffer(GL_COLOR_ATTACHMENT2_EXT);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);		// Clear The Screen And The Depth Buffer

	glDrawBuffers(4, buffers);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);


	
	// Select The Modelview Matrix
	glMatrixMode(GL_MODELVIEW);			

	// Reset The Modelview Matrix
	glLoadIdentity();

	g_camera.set_perspective();

	// Grab view matrix
	matrix44 modelview_mat;
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview_mat.m_data);
	matrix44 view_mat_inv = modelview_mat.inverse();

	matrix44 proj_mat;
	glGetFloatv(GL_PROJECTION_MATRIX, proj_mat.m_data);
	matrix44 proj_mat_inv = proj_mat.inverse();

	matrix44 viewproj = proj_mat * modelview_mat;
	matrix44 viewproj_inv = viewproj.inverse();

	glPushAttrib(GL_CURRENT_BIT | GL_POLYGON_BIT | GL_LIGHTING_BIT | GL_EVAL_BIT);
	
	glEnableClientState(GL_VERTEX_ARRAY);

	glEnable(GL_LIGHTING);

//#define RENDER_PRE_PASS
#if defined (RENDER_PRE_PASS)
	glDrawBuffer(GL_NONE);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_TRUE);
	draw_geometry(true);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_FALSE);
#endif


	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	
	draw_geometry(false, &modelview_mat);
	glDepthMask(GL_TRUE);

	glDisable(GL_LIGHTING);

	glPopAttrib();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);

	g_framebuffer_object_base_pass.unbind();

	// Step Two: Apply Lighting

	//draw_lights(&modelview_mat, &proj_mat_inv);
	draw_lights(&modelview_mat, &viewproj_inv, &proj_mat_inv);

	// Step Three: Framebuffer effects


	draw_final_scene();


	
	//glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
	//glDisable(GL_TEXTURE_RECTANGLE_ARB);

	SDL_GL_SwapBuffers();
}



static void printOpenGLError()
{
}

static void printShaderInfoLog(GLuint p_shader)
{
	GLint infologLen = 0;
	GLint charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(p_shader, GL_INFO_LOG_LENGTH, &infologLen);
	printOpenGLError();
	if (infologLen > 0) {
		infoLog = (GLchar *)malloc(infologLen);
		if (infoLog == NULL) {
			exit(1);
		}

		glGetShaderInfoLog(p_shader, infologLen, &charsWritten, infoLog);
		printf("InfoLog:\n%s\n\n", infoLog);
		free(infoLog);
	}
	printOpenGLError();
}

unsigned long render_texture_bind(void *p_pixel_data, unsigned long p_h, unsigned long p_w)
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
	gluBuild2DMipmaps( GL_TEXTURE_2D, 3, p_w, p_h, GL_RGB, GL_UNSIGNED_BYTE, p_pixel_data);

	return id;
}

void render_texture_unbind(unsigned long p_texture_id)
{
	glDeleteTextures( 1, (GLuint *)&p_texture_id );
}