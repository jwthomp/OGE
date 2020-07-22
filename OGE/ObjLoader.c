/*===================================================================================================
**
**	Author	:	Robert Bateman
**	E-Mail	:	rbateman@bournemouth.ac.uk
**	Brief	:	This Sourcefile is part of a series explaining how to load and render Alias Wavefront
**				Files somewhat efficently. If you are simkply after a reliable Obj Loader, then I would
**				Recommend version8; or possibly version9 and the supplied loader for extreme efficency.
**
**	Note	:	This Source Code is provided as is. No responsibility is accepted by myself for any
**				damage to hardware or software caused as a result of using this code. You are free to
**				make any alterations you see fit to this code for your own purposes, and distribute
**				that code either as part of a source code or binary executable package. All I ask is
**				for a little credit somewhere for my work!
** 
**				Any source improvements or bug fixes should be e-mailed to myself so I can update the
**				code for the greater good of the community at large. Credit will be given to the 
**				relevant people as always....
**				
**
**				Copyright (c) Robert Bateman, www.robthebloke.org, 2004
**
**				
**				National Centre for Computer Animation,
**				Bournemouth University,
**				Talbot Campus,
**				Bournemouth,
**				BH3 72F,
**				United Kingdom
**				ncca.bournemouth.ac.uk
**	
**
===================================================================================================*/

/*===================================================================================================
**
**												Includes
**
**=================================================================================================*/

/*
**	Need the windows headers to be included for openGL when coding on WIN32
*/
#ifdef WIN32
#include <windows.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "ObjLoader.h"
#include "tgaLoader.h"

#ifndef APIENTRY
#  if defined(__CYGWIN__) || defined(__MINGW32__)
#    define APIENTRY __stdcall
#  elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY APIENTRY
#endif

/*===================================================================================================
**
**										Vertex Buffer Object Extensions
**
**=================================================================================================*/

#ifndef GL_ARB_vertex_buffer_object
#define GL_ARB_vertex_buffer_object 1

#define GL_BUFFER_SIZE_ARB 0x8764
#define GL_BUFFER_USAGE_ARB 0x8765
#define GL_ARRAY_BUFFER_ARB 0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB 0x8893
#define GL_ARRAY_BUFFER_BINDING_ARB 0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB 0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB 0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB 0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB 0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB 0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB 0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB 0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB 0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB 0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB 0x889E
#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB 0x889F
#define GL_READ_ONLY_ARB 0x88B8
#define GL_WRITE_ONLY_ARB 0x88B9
#define GL_READ_WRITE_ARB 0x88BA
#define GL_BUFFER_ACCESS_ARB 0x88BB
#define GL_BUFFER_MAPPED_ARB 0x88BC
#define GL_BUFFER_MAP_POINTER_ARB 0x88BD
#define GL_STREAM_DRAW_ARB 0x88E0
#define GL_STREAM_READ_ARB 0x88E1
#define GL_STREAM_COPY_ARB 0x88E2
#define GL_STATIC_DRAW_ARB 0x88E4
#define GL_STATIC_READ_ARB 0x88E5
#define GL_STATIC_COPY_ARB 0x88E6
#define GL_DYNAMIC_DRAW_ARB 0x88E8
#define GL_DYNAMIC_READ_ARB 0x88E9
#define GL_DYNAMIC_COPY_ARB 0x88EA

typedef int GLsizeiptrARB;
typedef int GLintptrARB;

typedef void (GLAPIENTRY * PFNGLBINDBUFFERARBPROC) (GLenum target, GLuint buffer);
typedef void (GLAPIENTRY * PFNGLBUFFERDATAARBPROC) (GLenum target, GLsizeiptrARB size, const void* data, GLenum usage);
typedef void (GLAPIENTRY * PFNGLBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void* data);
typedef void (GLAPIENTRY * PFNGLDELETEBUFFERSARBPROC) (GLsizei n, const GLuint* buffers);
typedef void (GLAPIENTRY * PFNGLGENBUFFERSARBPROC) (GLsizei n, GLuint* buffers);
typedef void (GLAPIENTRY * PFNGLGETBUFFERPARAMETERIVARBPROC) (GLenum target, GLenum pname, GLint* params);
typedef void (GLAPIENTRY * PFNGLGETBUFFERPOINTERVARBPROC) (GLenum target, GLenum pname, GLvoid ** params);
typedef void (GLAPIENTRY * PFNGLGETBUFFERSUBDATAARBPROC) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, void* data);
typedef GLboolean (GLAPIENTRY * PFNGLISBUFFERARBPROC) (GLuint buffer);
typedef GLvoid * (GLAPIENTRY * PFNGLMAPBUFFERARBPROC) (GLenum target, GLenum access);
typedef GLboolean (GLAPIENTRY * PFNGLUNMAPBUFFERARBPROC) (GLenum target);

PFNGLBINDBUFFERARBPROC				glBindBufferARB				=NULL;
PFNGLBUFFERDATAARBPROC				glBufferDataARB				=NULL;
PFNGLBUFFERSUBDATAARBPROC			glBufferSubDataARB			=NULL;
PFNGLDELETEBUFFERSARBPROC			glDeleteBuffersARB			=NULL;
PFNGLGENBUFFERSARBPROC				glGenBuffersARB				=NULL;
PFNGLGETBUFFERPARAMETERIVARBPROC	glGetBufferParameterivARB	=NULL;
PFNGLGETBUFFERPOINTERVARBPROC		glGetBufferPointervARB		=NULL;
PFNGLGETBUFFERSUBDATAARBPROC		glGetBufferSubDataARB		=NULL;
PFNGLISBUFFERARBPROC				glIsBufferARB				=NULL;
PFNGLMAPBUFFERARBPROC				glMapBufferARB				=NULL;
PFNGLUNMAPBUFFERARBPROC				glUnmapBufferARB			=NULL;

#endif


/*===================================================================================================
**
**										Multi-Texturing Extensions
**
**=================================================================================================*/

#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture                 1

#define GL_ACTIVE_TEXTURE_ARB               0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB        0x84E1
#define GL_MAX_ACTIVE_TEXTURES_ARB          0x84E2
#define GL_TEXTURE0_ARB                     0x84C0
#define GL_TEXTURE1_ARB                     0x84C1
#define GL_TEXTURE2_ARB                     0x84C2
#define GL_TEXTURE3_ARB                     0x84C3
#define GL_TEXTURE4_ARB                     0x84C4
#define GL_TEXTURE5_ARB                     0x84C5
#define GL_TEXTURE6_ARB                     0x84C6
#define GL_TEXTURE7_ARB                     0x84C7
#define GL_TEXTURE8_ARB                     0x84C8
#define GL_TEXTURE9_ARB                     0x84C9
#define GL_TEXTURE10_ARB                    0x84CA
#define GL_TEXTURE11_ARB                    0x84CB
#define GL_TEXTURE12_ARB                    0x84CC
#define GL_TEXTURE13_ARB                    0x84CD
#define GL_TEXTURE14_ARB                    0x84CE
#define GL_TEXTURE15_ARB                    0x84CF
#define GL_TEXTURE16_ARB                    0x84D0
#define GL_TEXTURE17_ARB                    0x84D1
#define GL_TEXTURE18_ARB                    0x84D2
#define GL_TEXTURE19_ARB                    0x84D3
#define GL_TEXTURE20_ARB                    0x84D4
#define GL_TEXTURE21_ARB                    0x84D5
#define GL_TEXTURE22_ARB                    0x84D6
#define GL_TEXTURE23_ARB                    0x84D7
#define GL_TEXTURE24_ARB                    0x84D8
#define GL_TEXTURE25_ARB                    0x84D9
#define GL_TEXTURE26_ARB                    0x84DA
#define GL_TEXTURE27_ARB                    0x84DB
#define GL_TEXTURE28_ARB                    0x84DC
#define GL_TEXTURE29_ARB                    0x84DD
#define GL_TEXTURE30_ARB                    0x84DE
#define GL_TEXTURE31_ARB                    0x84DF

typedef void (APIENTRY * PFNGLACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLCLIENTACTIVETEXTUREARBPROC) (GLenum texture);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DARBPROC) (GLenum texture, GLdouble s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1DVARBPROC) (GLenum texture, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FARBPROC) (GLenum texture, GLfloat s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IARBPROC) (GLenum texture, GLint s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1IVARBPROC) (GLenum texture, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SARBPROC) (GLenum texture, GLshort s);
typedef void (APIENTRY * PFNGLMULTITEXCOORD1SVARBPROC) (GLenum texture, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DARBPROC) (GLenum texture, GLdouble s, GLdouble t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2DVARBPROC) (GLenum texture, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FARBPROC) (GLenum texture, GLfloat s, GLfloat t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IARBPROC) (GLenum texture, GLint s, GLint t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2IVARBPROC) (GLenum texture, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SARBPROC) (GLenum texture, GLshort s, GLshort t);
typedef void (APIENTRY * PFNGLMULTITEXCOORD2SVARBPROC) (GLenum texture, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DARBPROC) (GLenum texture, GLdouble s, GLdouble t, GLdouble r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3DVARBPROC) (GLenum texture, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FARBPROC) (GLenum texture, GLfloat s, GLfloat t, GLfloat r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IARBPROC) (GLenum texture, GLint s, GLint t, GLint r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3IVARBPROC) (GLenum texture, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SARBPROC) (GLenum texture, GLshort s, GLshort t, GLshort r);
typedef void (APIENTRY * PFNGLMULTITEXCOORD3SVARBPROC) (GLenum texture, const GLshort *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DARBPROC) (GLenum texture, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4DVARBPROC) (GLenum texture, const GLdouble *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FARBPROC) (GLenum texture, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4FVARBPROC) (GLenum texture, const GLfloat *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IARBPROC) (GLenum texture, GLint s, GLint t, GLint r, GLint q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4IVARBPROC) (GLenum texture, const GLint *v);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SARBPROC) (GLenum texture, GLshort s, GLshort t, GLshort r, GLshort q);
typedef void (APIENTRY * PFNGLMULTITEXCOORD4SVARBPROC) (GLenum texture, const GLshort *v);

PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;


#endif /* GL_ARB_multitexture */


/*===================================================================================================
**
**										Cube Map Extensions
**
**=================================================================================================*/


#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1

#define GL_NORMAL_MAP_ARB					0x8511
#define GL_REFLECTION_MAP_ARB				0x8512
#define GL_TEXTURE_CUBE_MAP_ARB				0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB		0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB	0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB	0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB	0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB  0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB		0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB	0x851C

#endif /* GL_ARB_texture_cube_map */



/*===================================================================================================
**
**										Texture Combiner Extensions
**
**=================================================================================================*/

#ifndef GL_EXT_texture_env_combine
#define GL_EXT_texture_env_combine			1

#define GL_COMBINE_EXT						0x8570
#define GL_COMBINE_RGB_EXT					0x8571
#define GL_COMBINE_ALPHA_EXT				0x8572
#define GL_SOURCE0_RGB_EXT					0x8580
#define GL_SOURCE1_RGB_EXT					0x8581
#define GL_SOURCE2_RGB_EXT					0x8582
#define GL_SOURCE0_ALPHA_EXT				0x8588
#define GL_SOURCE1_ALPHA_EXT				0x8589
#define GL_SOURCE2_ALPHA_EXT				0x858A
#define GL_OPERAND0_RGB_EXT					0x8590
#define GL_OPERAND1_RGB_EXT					0x8591
#define GL_OPERAND2_RGB_EXT					0x8592
#define GL_OPERAND0_ALPHA_EXT				0x8598
#define GL_OPERAND1_ALPHA_EXT				0x8599
#define GL_OPERAND2_ALPHA_EXT				0x859A
#define GL_RGB_SCALE_EXT					0x8573
#define GL_ADD_SIGNED_EXT					0x8574
#define GL_INTERPOLATE_EXT					0x8575
#define GL_CONSTANT_EXT						0x8576
#define GL_PRIMARY_COLOR_EXT				0x8577
#define GL_PREVIOUS_EXT						0x8578

#endif /* GL_EXT_texture_env_combine */



/*===================================================================================================
**
**										Texture Env Dot3 Extensions
**
**=================================================================================================*/

#ifndef GL_EXT_texture_env_dot3
#define GL_EXT_texture_env_dot3 1

#define GL_DOT3_RGB_EXT						0x8740
#define GL_DOT3_RGBA_EXT					0x8741

#endif



/*===================================================================================================
**
**										OpenGL Extension Initialisation
**
**=================================================================================================*/

#ifdef WIN32

void InitOpenGL_Extensions()
{
	if (!glBindBufferARB)
	{
		const char* ext = glGetString(GL_EXTENSIONS);
		unsigned int i=0;
		while (ext[i] != '\0') {
			if (ext[i] == ' ') {
				putc('\n',stdout);
			} else
				putc(ext[i],stdout);
			++i;
		}

		glBindBufferARB				= (PFNGLBINDBUFFERARBPROC)			wglGetProcAddress("glBindBufferARB");
		glBufferDataARB				= (PFNGLBUFFERDATAARBPROC)			wglGetProcAddress("glBufferDataARB");
		glBufferSubDataARB			= (PFNGLBUFFERSUBDATAARBPROC)		wglGetProcAddress("glBufferSubDataARB");
		glDeleteBuffersARB			= (PFNGLDELETEBUFFERSARBPROC)		wglGetProcAddress("glDeleteBuffersARB");
		glGenBuffersARB				= (PFNGLGENBUFFERSARBPROC)			wglGetProcAddress("glGenBuffersARB");
		glGetBufferParameterivARB	= (PFNGLGETBUFFERPARAMETERIVARBPROC)wglGetProcAddress("glGetBufferParameterivARB");
		glGetBufferPointervARB		= (PFNGLGETBUFFERPOINTERVARBPROC)	wglGetProcAddress("glGetBufferPointervARB");
		glGetBufferSubDataARB		= (PFNGLGETBUFFERSUBDATAARBPROC)	wglGetProcAddress("glGetBufferSubDataARB");
		glIsBufferARB				= (PFNGLISBUFFERARBPROC)			wglGetProcAddress("glIsBufferARB");
		glMapBufferARB				= (PFNGLMAPBUFFERARBPROC)			wglGetProcAddress("glMapBufferARB");
		glUnmapBufferARB			= (PFNGLUNMAPBUFFERARBPROC)			wglGetProcAddress("glUnmapBufferARB");

		glActiveTextureARB			= (PFNGLACTIVETEXTUREARBPROC)		wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB	= (PFNGLCLIENTACTIVETEXTUREARBPROC)	wglGetProcAddress("glClientActiveTextureARB");

		if (!glBindBufferARB)
		{	
			fprintf(stderr,"[ERROR] This hardware does not seem to support Vertex Buffer Objects. You might want to ask for just vertex arrays next time....\n");
			exit(0);
		}

		if (!glActiveTextureARB)
		{	
			fprintf(stderr,"[ERROR] This hardware does not seem to support multi-texturing. Buy a new graphics card!\n");
			exit(0);
		}
	}
}

#else

void InitOpenGL_Extensions()
{
}

#endif


/*===================================================================================================
**
**										Internal Data Structures
**
**=================================================================================================*/


/*
**	To start with, only deal with the obj file as a list of points. Essentually,
**	if we are able to draw the points out they should roughly describe the mesh.
**	So, lets look for the 'v' flag that says that the 3 floats after it describes a
**	Vertex. Essentually then, we need to have a structure for a vertex.
*/
typedef struct
{
	float x,			/*	The x component of the vertex position	*/
		  y,			/*	The y component of the vertex position	*/
		  z;			/*	The z component of the vertex position	*/
} ObjVertex;


/*
**	Now, the normal vector flag 'vn' could also do with a structure to represent it,
**	this will ultimately look very similar to the Vertex structure seeing as they have
**	similar data
*/
typedef struct
{
	float x,			/*	The x component of the normal vector	*/
		  y,			/*	The y component of the normal vector	*/
		  z;			/*	The z component of the normal vector	*/
} ObjNormal;


/*
**	Now, the normal vector flag 'vn' could also do with a structure to represent it,
**	this will ultimately look very similar to the Vertex structure seeing as they have
**	similar data
*/
typedef struct
{
	float r,			/*	The x component of the normal vector	*/
		g,			/*	The y component of the normal vector	*/
		b;			/*	The z component of the normal vector	*/
} ObjColour;


/*
**	Now, the normal vector flag 'vn' could also do with a structure to represent it,
**	this will ultimately look very similar to the Vertex structure seeing as they have
**	similar data
*/
typedef struct
{
	float x,			/*	The x component of the tangent vector	*/
		  y,			/*	The y component of the tangent vector	*/
		  z,			/*	The z component of the tangent vector	*/
		  w;			/*	The w component of the tangent vector	*/
} ObjTangent;



/*
**	A Texturing co-ordinate usually has only two values, the u and the v. Make a third
**	struct for this fact.
*/
typedef struct
{
	float u,			/*	The u parametric texturing co-ordinate	*/
		  v;			/*	The v parametric texturing co-ordinate	*/
} ObjTexCoord;


/*
**	Each face is composed of a number of 'corners'. At each 'corner' there will be at a
**	minimum, one vertex coordinate, and possibly a normal vector and a texturing co-ordinate.
**	Seeing as quite often a specific vertex is used by a number of faces, rather than store the
**	vertices in the face structure itself, we can use an integer reference to the position of the
**	vertex in the array. Basically it allows us the ability of storing the data as a 4byte value
**	rather than the 12 needed for a vertex.
**
**	To summaraise,
**
**		Each face is comprised of a number of 'corners', in this case the number will be held in
**	the value "m_iVertexCount". Taking into account the worst case scenario, for each corner, we
**	could have a vertex, normal and a texturing co-ordinate (usual actually). So we may need to hold
**	3 * m_iVertexCount number of indices for our faces. It kindof makes sense to basically dynamically
**	allocate an array to hold the vertex indicies, an array for the normal indices and one for the uv
**	coords. These are m_aVertexIndices, m_aNormalIndices, and m_aTexCoordIndicies respectivley.
**
**	So our full face structure looks a bit like this :
*/
typedef struct
{
	unsigned int  m_aVertexIndices[3],		/*	array of indicies that reference the vertex array in the mesh	*/
				  m_aNormalIndices[3],		/*	array of indicies that reference the normal array in the mesh	*/
				  m_aTexCoordIndicies[3];	/*	array of indicies that reference the uv coordinate array in the mesh	*/

} ObjFace;


/*
**	Each mesh is held as a structure with arrays of it's vertices, normals and texturing co-ordinates.
**	An Array of faces then references the arrays. There is also a pointer to the next node in the linked
**	list.
**
**	As a space saving excersise, i've unioned a number of variables together within the data structure.
**	This allows me to pack to variables into the same memory location thus saving memory. For example, 
**	the m_iNumberOfVertices variable is only used in immediate mode, once the data has been converted
**	to vertex arrays this become redundant. Since m_iVBO_Size is only used when the data is a vetex buffer
**	object, it may as well share the same memory location as m_iNumberOfVertices since the two vars will
**	never be used at the same time.
*/
typedef struct _ObjMesh
{
	ObjFile			 m_iMeshID;					/*	the ID of the mesh	*/
	unsigned char	 m_iMode;					/*	Display mode --> Either 0 - normal, 1 - display list or 2 - vertex array */

	ObjVertex		*m_aVertexArray;			/*	Array of vertices that make up this mesh	*/
	ObjNormal		*m_aNormalArray;			/*	Array of normals that make up this mesh		*/
	ObjTexCoord		*m_aTexCoordArray;			/*	Array of texturing co-ordinates that make up this mesh */
	ObjNormal		*m_aBiNormalArray;			/*	Array of bi-normals that make up this mesh		*/
	ObjColour		*m_aColourArray;			/*  Array of vertex colours - this actually holds the light vector for each vertex */
	union {
		ObjTangent		*m_aTangentArray;		/*	Array of tangents that make up this mesh		*/
		ObjFace			*m_aFaces;				/*	Array of faces that make up this mesh */
	};
	unsigned int	*m_aIndices;				/*	Array of triangle face indices  */
	unsigned int	 m_iNumberOfFaces;			/*	The number of faces in the m_aFaces array			*/


	union {
		unsigned int	 m_iDisplayListNum;		/*	Used only if the mode is set to display list - holds the display list number */
		unsigned int	 m_iVBONum;				/*	Used only if the mode is set to Vertex Buffer Object - holds the VBO data array ID */
		unsigned int	 m_iNumberOfTexCoords;	/*	The number of uv's in the m_aTexCoordArray array	*/
	};

	union {
		unsigned int	 m_iNumberOfNormals;	/*	The number of normals in the m_aNormalArray array	*/
		unsigned int	 m_iVBO_ElemNum;		/*	Used only if the mode is set to Vertex Buffer Object - holds the VBO element array ID */
	};

	union {
		unsigned int	 m_iNumberOfVertices;	/*	The number of vertices in the m_aVertexArray array	*/
		unsigned int	 m_iVBO_Size;			/*	Used only if the mode is set to Vertex Buffer Object - holds the VBO element array ID */
	};

	unsigned int m_iVBO_Dynamic;				/* A second VBO is used for all dynamic streamed data */

	unsigned int m_BumpTexture;
	unsigned int m_BaseTexture;

	struct _ObjMesh *m_pNext;				/*	Each mesh will be stored internally as a node on a linked list */

} ObjMesh;


/*===================================================================================================
**
**										Global Variables
**
**=================================================================================================*/

/*
**	The global head of the linked list of meshes. This is a linked list because it is possible that you will be
**	loading and deleting meshes during the course of the programs execution.
*/
ObjMesh *g_LinkedListHead = NULL;

/* 
**	This is used to generate a unique ID for each Obj File
*/
unsigned int g_ObjIdGenerator=0;

/*
**	This function will take a pointer to the mesh and the number of extra triangles that
**	are required. It will then resize the polygon face array within the mesh and return
**	the index of the first new face that was allocated.
*/
unsigned int ResizeTriangleArray(ObjMesh *pMesh,unsigned int iExtraTriangleCount)
{
	/*
	**	Create a temporary pointer to the old face array in the mesh
	*/
	ObjFace *pTempFaces = pMesh->m_aFaces;

	/*
	**	Keep track of the old face count
	*/
	unsigned int iTemp  = pMesh->m_iNumberOfFaces;

	/*
	**	Increase the number of faces that will be stored in the mesh
	*/
	pMesh->m_iNumberOfFaces += iExtraTriangleCount;

	/*
	**	Allocate the number of faces now required by this mesh. Note, you could use
	**	realloc for this, but it's just my personal preference to do it this way.
	*/
	pMesh->m_aFaces = (ObjFace*)malloc(pMesh->m_iNumberOfFaces*sizeof(ObjFace));

	/*
	**	If we assert here then the memory allocation for the meshes failed
	*/
	assert(pMesh->m_aFaces);

	/*
	**	Copy across the data that was stored in the old array
	*/
	memcpy(pMesh->m_aFaces,pTempFaces,iTemp*sizeof(ObjFace));

	/*
	**	Delete the data held in the previous array
	*/
	free(pTempFaces);

	memset(&pMesh->m_aFaces[ iTemp ],0,iExtraTriangleCount*sizeof(ObjFace) );

	/*
	**	return the old size of the array (thats the same as the index of the first new triangle).
	*/
	return iTemp;

}


/*
**	This function is only called from within the *.c file and is used to create an ObjMesh structure and
**	initialise its values (adds the mesh to the linked list also).
*/
ObjMesh *MakeOBJ( void )
{

	/*
	**	The pointer we will create the mesh at the end of
	*/
	ObjMesh *pMesh = NULL;


	pMesh = (ObjMesh*) malloc (sizeof(ObjMesh));

	/*	If the program asserts here, then there was a memory allocation failure	*/
	assert(pMesh);

	/*
	**	Initialise all pointers to NULL
	*/
	pMesh->m_aFaces				= NULL;
	pMesh->m_aNormalArray		= NULL;
	pMesh->m_aTangentArray		= NULL;
	pMesh->m_aBiNormalArray		= NULL;
	pMesh->m_aTexCoordArray		= NULL;
	pMesh->m_aColourArray		= NULL;
	pMesh->m_aVertexArray		= NULL;
	pMesh->m_aIndices			= NULL;
	pMesh->m_iNumberOfFaces		= 0;
	pMesh->m_iNumberOfNormals	= 0;
	pMesh->m_iNumberOfTexCoords = 0;
	pMesh->m_iNumberOfVertices	= 0;
	pMesh->m_iMode				= 0;
	pMesh->m_iMeshID			= ++g_ObjIdGenerator;


	/*
	**	Insert the mesh at the beginning of the linked list
	*/
	pMesh->m_pNext				= g_LinkedListHead;
	g_LinkedListHead			= pMesh;

	return pMesh;
}

void GetIndices(char *pString, unsigned int *pVert, unsigned int *pNorm, unsigned int *pUV)
{
	/*
	**	Use 3 pointers to hold references to the bits of the string where the indices start
	**	for the vertex, normal and texturing co-ordinate respectivly
	*/
	char *pV = pString,
		 *pN = NULL,
		 *pT = NULL;

	/*
	**	Walk through the string a character at a time looking for the '/' character. The first one
	**	we find will may have the uv - coord index behind it. The '/' character will be removed by
	**	setting it to NULL, If for example the texturing co-ord is missing, then you'd expect "//" to
	**	appear. This while loop would set both of those to NULL, that would give you a NULL string
	**	for the uv coord. This allows us to test the string later to see if there is or isn't a
	**	value there.
	*/
	while( *pString != '\0' )
	{
		if(*pString == '/')
		{
			*pString = '\0';
			if(!pT)
			{
				pT = pString+1;
			}
			else
			{
				pN = pString+1;
			}
		}
		pString++;
	}

	/*
	**	Read the vertex index
	*/
	sscanf(pV,"%d",pVert);

	/*
	**	If a normal is there, then read it's value, otherwise set the default of 1
	**	(1 is used because we will decrease the value by 1 later thus giving us 0)
	*/
	if( pN == NULL || *pN == '\0' )
	{
		*pNorm = 1;
	}
	else
	{
		sscanf(pN,"%d",pNorm);
	}

	/*
	**	If a texture co-ordinate is there, then read it's value, otherwise set the default of 1
	**	(1 is used because we will decrease the value by 1 later thus giving us 0)
	*/
	if( pT == NULL || *pT == '\0' )
	{
		*pUV = 1;
	}
	else
	{
		sscanf(pT,"%d",pUV);
	}
}

ObjFile LoadOBJ(const char *filename)
{
	ObjMesh *pMesh=NULL;
	unsigned int vc=0,nc=0,tc=0,fc=0;
	char buffer[256];
	FILE *fp = NULL;

	/*
	**	Open the file for reading
	*/
	fp = fopen(filename,"r");

	/*
	**	If the program asserted here, then the file could not be found or opened
	*/
	assert(fp);

	/*
	**	Create the mesh structure and add it to the linked list
	*/
	pMesh = MakeOBJ();

	/*
	**	Run through the whole file looking for the various flags so that we can count
	**	up how many data elements there are. This is done so that we can make one memory
	**	allocation for the meshes data and then run through the file once more, this time
	**	reading in the data. It's purely done to reduce system overhead of memory allocation due
	**	to otherwise needing to reallocate data everytime we read in a new element.
	*/
	while(!feof(fp))
	{
		memset(buffer,0,200);

		/*	Grab a line at a time	*/
		fgets(buffer,256,fp);

		/*	look for the 'vn' - vertex normal - flag	*/
		if( strncmp("vn ",buffer,3) == 0 )
		{
			++pMesh->m_iNumberOfNormals;
		}
		else

		/*	look for the 'vt' - texturing co-ordinate - flag  */
		if( strncmp("vt ",buffer,3) == 0 )
		{
			++pMesh->m_iNumberOfTexCoords;
		}
		else

		/*	look for the 'v ' - vertex co-ordinate - flag  */
		if( strncmp("v ",buffer,2) == 0 )
		{
			++pMesh->m_iNumberOfVertices;
		}
		else

		/*	look for the 'f ' - face - flag  */
		if( strncmp("f ",buffer,2) == 0 )
		{
			++pMesh->m_iNumberOfFaces;
		}
	}

	/*
	**	close the file for now....
	*/
	fclose(fp);

	/*
	**	Allocate the memory for the data arrays and check that it allocated ok
	*/
	pMesh->m_aVertexArray	= (ObjVertex*  )malloc( pMesh->m_iNumberOfVertices	* sizeof(ObjVertex)	  );
	assert(pMesh->m_aVertexArray);

	/*	there are occasionally times when the obj does not have any normals in it */
	if( pMesh->m_iNumberOfNormals > 0 )
	{
		pMesh->m_aNormalArray	= (ObjNormal*  )malloc( pMesh->m_iNumberOfNormals	* sizeof(ObjNormal)	  );
		assert(pMesh->m_aNormalArray);
	}

	/*	there are occasionally times when the obj does not have any tex coords in it */
	if( pMesh->m_iNumberOfTexCoords > 0 )
	{
		pMesh->m_aTexCoordArray = (ObjTexCoord*)malloc( pMesh->m_iNumberOfTexCoords	* sizeof(ObjTexCoord) );
		assert(pMesh->m_aTexCoordArray);
	}

	pMesh->m_aFaces			= (ObjFace*    )malloc( pMesh->m_iNumberOfFaces		* sizeof(ObjFace)	  );
	assert(pMesh->m_aFaces);

	/*
	**	Now we know how much data is contained in the file and we've allocated memory to hold it all.
	**	What we can therefore do now, is load up all of the data in the file easily.
	*/
	fp = fopen(filename,"r");

	while(!feof(fp))
	{
		memset(buffer,0,255);

		/*	Grab a line at a time	*/
		fgets(buffer,256,fp);

		/*	look for the 'vn' - vertex normal - flag	*/
		if( strncmp("vn ",buffer,3) == 0 )
		{
			sscanf((buffer+2),"%f%f%f",
							&pMesh->m_aNormalArray[ nc ].x,
							&pMesh->m_aNormalArray[ nc ].y,
							&pMesh->m_aNormalArray[ nc ].z);
			++nc;
		}
		else

		/*	look for the 'vt' - texturing co-ordinate - flag  */
		if( strncmp("vt ",buffer,3) == 0 )
		{
			sscanf((buffer+2),"%f%f",
							&pMesh->m_aTexCoordArray[ tc ].u,
							&pMesh->m_aTexCoordArray[ tc ].v);
			++tc;
		}
		else

		/*	look for the 'v ' - vertex co-ordinate - flag  */
		if( strncmp("v ",buffer,2) == 0 )
		{
			sscanf((buffer+1),"%f%f%f",
							&pMesh->m_aVertexArray[ vc ].x,
							&pMesh->m_aVertexArray[ vc ].y,
							&pMesh->m_aVertexArray[ vc ].z);
			++vc;
		}
		else

		/*	look for the 'f ' - face - flag  */
		if( strncmp("f ",buffer,2) == 0 )
		{
			/*
			**	some data for later....
			*/
			char *pSplitString = NULL;
			unsigned int i,ii = 0;
			unsigned int iNewTrianglesIndex = 0,
						 iNumberOfExtraTriangles = 0;
			unsigned int aFirstIndices[3],
						 aSecondIndices[3];

			ObjFace *pf = NULL;

			/*
			**	These next few lines are used to figure out how many '/' characters there
			**	are in the string. This gives us the information we need to find out how
			**	many vertices are used in this face (by dividing by two)
			*/
			for(i=0;i<strlen(buffer);i++)
			{
				if(buffer[i] == '/')
					ii++;
			}
			ii/=2;

			/*
			**	This is where it all gets confusing, What I'm going to do is to convert all polys
			**	With vertices above 3 to triangles. The number of triangles I require will be (ii-2).
			**	For example, with a poly that has vertices a,b,c,d,e; there will be 3 triangles comprised
			**	of verts (a,b,c), (a,c,d) and (a,d,e) respectively. We need to check the amount of triangles
			**	that will be needed for this polygon and resize the array accordingly.
			*/
			if( (ii-2) > 1 )
			{
				iNumberOfExtraTriangles = ii-3;
				iNewTrianglesIndex = ResizeTriangleArray(pMesh,iNumberOfExtraTriangles);
			}

			/*
			**	Pointer to the face we are currently dealing with. It's only used so that
			**	the code becomes more readable and I have less to type.
			*/
			pf = &pMesh->m_aFaces[ fc ];


			/*
			**	tokenise the string using strtok(). Basically this splits the string up
			**	and removes the spaces from each chunk. This way we only have to deal with
			**	one set of indices at a time for each of the poly's vertices.
			*/
			pSplitString = strtok((buffer+2)," \t\n");
			i=0;

			/*
			**	Get the vertex structures that we know WILL exists (there has to be a minimum of three)
			*/
			for(i=0;i<3;i++)
			{
				GetIndices( pSplitString,
							&pf->m_aVertexIndices[i],
							&pf->m_aNormalIndices[i],
							&pf->m_aTexCoordIndicies[i]);

				/* need to reduce the indices by 1 because array indices start at 0, obj starts at 1  */
				--pf->m_aTexCoordIndicies[i];
				--pf->m_aNormalIndices[i];
				--pf->m_aVertexIndices[i];

				/*
				**	In order to start constructing our triangles that are created after we split the poly,
				**	we need to store some of the indices that are going to be used for the extra additional
				**	triangles after this. Basically we store the first and last sets of indices from this
				**	first triangle becasue they will be used as the first & second sets for the next triangle.
				*/
				switch(i)
				{
				case 0:
					aFirstIndices[0] = pf->m_aVertexIndices[i];
					aFirstIndices[1] = pf->m_aNormalIndices[i];
					aFirstIndices[2] = pf->m_aTexCoordIndicies[i];
					break;

				case 2:
					aSecondIndices[0] = pf->m_aVertexIndices[i];
					aSecondIndices[1] = pf->m_aNormalIndices[i];
					aSecondIndices[2] = pf->m_aTexCoordIndicies[i];
					break;
				default:
					break;
				}

				pSplitString = strtok(NULL," \t\n");
			}
			++fc;

			/*
			**	Get the additional triangles that may be there
			*/
			for(i=0;i<iNumberOfExtraTriangles;i++)
			{
				pf = &pMesh->m_aFaces[ iNewTrianglesIndex + i ];

				GetIndices( pSplitString,
							&pf->m_aVertexIndices[2],
							&pf->m_aNormalIndices[2],
							&pf->m_aTexCoordIndicies[2]);

				/* need to reduce the indices by 1 because array indices start at 0, obj starts at 1  */
				--pf->m_aTexCoordIndicies[2];
				--pf->m_aNormalIndices[2];
				--pf->m_aVertexIndices[2];

				/*
				**	Now assign the other indices for the triangle
				*/
				pf->m_aVertexIndices[0]		= aFirstIndices[0];
				pf->m_aNormalIndices[0]		= aFirstIndices[1];
				pf->m_aTexCoordIndicies[0]	= aFirstIndices[2];

				pf->m_aVertexIndices[1]		= aSecondIndices[0];
				pf->m_aNormalIndices[1]		= aSecondIndices[1];
				pf->m_aTexCoordIndicies[1]	= aSecondIndices[2];

				/*
				**	Copy over the indices for the next triangle.
				*/
				aSecondIndices[0] = pf->m_aVertexIndices[2];
				aSecondIndices[1] = pf->m_aNormalIndices[2];
				aSecondIndices[2] = pf->m_aTexCoordIndicies[2];

				pSplitString = strtok(NULL," \t\n");
			}
		}
	}

	fclose(fp);

	return pMesh->m_iMeshID;
}

/*
**	This function generates normals for the specified mesh. The data MUST
**	be stored in vertex arrays for this routine to work.
*/
void CalculateNormalArray(ObjMesh* pMesh)
{
	{
		unsigned int* f = pMesh->m_aIndices;
		unsigned int* end = f + (pMesh->m_iNumberOfFaces);

		memset(pMesh->m_aNormalArray,0,sizeof(ObjNormal)*pMesh->m_iVBO_Size);

		/*
		*	Loop through each face in the mesh
		*/
		for( ; f != end; f+=3 )
		{
			ObjVertex* v1 = pMesh->m_aVertexArray + f[0];
			ObjVertex* v2 = pMesh->m_aVertexArray + f[1];
			ObjVertex* v3 = pMesh->m_aVertexArray + f[2];

			ObjNormal* n1 = pMesh->m_aNormalArray + f[0];
			ObjNormal* n2 = pMesh->m_aNormalArray + f[1];
			ObjNormal* n3 = pMesh->m_aNormalArray + f[2];

			ObjNormal e1 = {
				v1->x - v2->x,
				v1->y - v2->y,
				v1->z - v2->z
			};

			ObjNormal e2 = {
				v3->x - v2->x,
				v3->y - v2->y,
				v3->z - v2->z
			};

			ObjNormal e1_cross_e2 = {
				e2.y * e1.z - e2.z * e1.y, 
				e2.z * e1.x - e2.x * e1.z,
				e2.x * e1.y - e2.y * e1.x
			};

			float it = 1.0f/((float)sqrt( e1_cross_e2.x*e1_cross_e2.x + 
										  e1_cross_e2.y*e1_cross_e2.y + 
										  e1_cross_e2.z*e1_cross_e2.z ));

			e1_cross_e2.x *= it;
			e1_cross_e2.y *= it;
			e1_cross_e2.z *= it;
			
			n1->x += e1_cross_e2.x;
			n1->y += e1_cross_e2.y;
			n1->z += e1_cross_e2.z;

			n2->x += e1_cross_e2.x;
			n2->y += e1_cross_e2.y;
			n2->z += e1_cross_e2.z;

			n3->x += e1_cross_e2.x;
			n3->y += e1_cross_e2.y;
			n3->z += e1_cross_e2.z;
		}
	}

	{
		ObjNormal* n = pMesh->m_aNormalArray;
		ObjNormal* end = n + pMesh->m_iVBO_Size;

		for( ; n != end; ++n )
		{
			
			float it = 1.0f/((float)sqrt( n->x * n->x + 
										  n->y * n->y + 
										  n->z * n->z ));
			n->x *= it;
			n->y *= it;
			n->z *= it;
		}
	}
}

/*
**	This function generates tangents for the specified mesh. 
**	The data MUST be stored in vertex arrays for this routine to work.
*/
void CalculateTangentArray(ObjMesh* pMesh)
{
	unsigned int a,count;

	/* generate a temporary array for the S & T vectors */
	ObjVertex *tan1 = (ObjVertex*)malloc( pMesh->m_iVBO_Size * 2 * sizeof(ObjVertex) );

	/* The 2nd array */
	ObjVertex *tan2 = tan1 + pMesh->m_iVBO_Size;

	/* zero the tangent memory */
	memset(tan1, 0, pMesh->m_iVBO_Size * sizeof(ObjVertex) * 2);

	/* loop through each triangle to calculate the S & T vectors on that triangle */
	for ( a = 0; a < pMesh->m_iNumberOfFaces; a+=3)
	{
		/*
		 *	get 3 indicies into the vertex array for the triangle
		 */
		unsigned int i1 = pMesh->m_aIndices[a+0];
		unsigned int i2 = pMesh->m_aIndices[a+1];
		unsigned int i3 = pMesh->m_aIndices[a+2];

		/*
		 *	Get pointers to vertices & texcoords used in the triangle
		 */
		ObjVertex* v1 = pMesh->m_aVertexArray + i1;
		ObjVertex* v2 = pMesh->m_aVertexArray + i2;
		ObjVertex* v3 = pMesh->m_aVertexArray + i3;

		ObjTexCoord* w1 = pMesh->m_aTexCoordArray + i1;
		ObjTexCoord* w2 = pMesh->m_aTexCoordArray + i2;
		ObjTexCoord* w3 = pMesh->m_aTexCoordArray + i3;

		/*
		 *	Calculate the S & T vectors
		 */
		float x1 = v2->x - v1->x;
		float x2 = v3->x - v1->x;
		float y1 = v2->y - v1->y;
		float y2 = v3->y - v1->y;
		float z1 = v2->z - v1->z;
		float z2 = v3->z - v1->z;

		float s1 = w2->u - w1->u;
		float s2 = w3->u - w1->u;
		float t1 = w2->v - w1->v;
		float t2 = w3->v - w1->v;

		float r = 1.0F / (s1 * t2 - s2 * t1);

		ObjVertex sdir = {
			(t2 * x1 - t1 * x2) * r, 
			(t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r
		};

		ObjVertex tdir = {
			(s1 * x2 - s2 * x1) * r, 
			(s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r
		};

		tan1[i1].x += sdir.x;
		tan1[i1].y += sdir.y;
		tan1[i1].z += sdir.z;

		tan1[i2].x += sdir.x;
		tan1[i2].y += sdir.y;
		tan1[i2].z += sdir.z;

		tan1[i3].x += sdir.x;
		tan1[i3].y += sdir.y;
		tan1[i3].z += sdir.z;


		tan2[i1].x += tdir.x;
		tan2[i1].y += tdir.y;
		tan2[i1].z += tdir.z;

		tan2[i2].x += tdir.x;
		tan2[i2].y += tdir.y;
		tan2[i2].z += tdir.z;

		tan2[i3].x += tdir.x;
		tan2[i3].y += tdir.y;
		tan2[i3].z += tdir.z;

	}

	count = pMesh->m_iVBO_Size;
	for (a = 0; a < count; a++)
	{
		ObjNormal nnt;
		ObjNormal* n = pMesh->m_aNormalArray + a;
		ObjVertex* t = tan1 + a;
		float il;
		float nt = ( n->x * t->x + n->y * t->y + n->z * t->z );

		nnt.x = t->x - nt * n->x;
		nnt.y = t->y - nt * n->y;
		nnt.z = t->z - nt * n->z;

		il = 1.0f/((float)sqrt( nnt.x*nnt.x + nnt.y*nnt.y + nnt.z*nnt.z ));

		// Gram-Schmidt orthogonalize
		pMesh->m_aTangentArray[a].x = il * nnt.x;
		pMesh->m_aTangentArray[a].y = il * nnt.y;
		pMesh->m_aTangentArray[a].z = il * nnt.z;

		// Calculate handedness
		{
			ObjNormal n_cross_t = {
				n->y * t->z - n->z * t->y, 
				n->z * t->x - n->x * t->z,
				n->x * t->y - n->y * t->x
			};

			float nt_dot_tan2 = 
				n_cross_t.x * tan2[a].x +
				n_cross_t.y * tan2[a].y +
				n_cross_t.z * tan2[a].z;

			pMesh->m_aTangentArray[a].w = (nt_dot_tan2 < 0.0f) ? -1.0f : 1.0f;

		}
	}

	free(tan1);
}

/*
**	This function generates bi-normals for the specified mesh. This is done
**	by simply performing a cross product between the normal & tangent.
**	The data MUST be stored in vertex arrays for this routine to work.
*/
void CalculateBiNormalArray(ObjMesh* pMesh)
{
	ObjTangent* t  = pMesh->m_aTangentArray;
	ObjNormal*  b  = pMesh->m_aBiNormalArray;
	ObjNormal*  n  = pMesh->m_aNormalArray;
	ObjNormal* end = n + pMesh->m_iVBO_Size;

	for( ; n != end; ++n,++b,++t )
	{
		b->x = n->y * t->z - n->z * t->y;
		b->y = n->z * t->x - n->x * t->z;
		b->z = n->x * t->y - n->y * t->x;		
	}
}

/*
**	This function renders the mesh either using glBegin/glEnd, display lists,
**	vertex arrays, or vertex buffer objects. 
*/
void DrawOBJ(ObjFile id)
{
	/*
	**	Because the meshes are on a linked list, we first need to find the
	**	mesh with the specified ID number so traverse the list.
	*/
	ObjMesh *pMesh = g_LinkedListHead;

	while(pMesh && pMesh->m_iMeshID != id)
	{
		pMesh = pMesh->m_pNext;
	}

	/*
	**	Check to see if the mesh ID is valid.
	*/
	if(pMesh != NULL)
	{
		/*
		**	Select which drawing mode to use
		*/
		switch(pMesh->m_iMode)
		{
		case 0:
			{
				/*
				**	In order to draw our mesh, we would basically like something of the following :
				**
				**	for_each_polygon_in_mesh
				**	{
				**		start_drawing_poly;
				**		for_each_vertex_in_the_poly
				**		{
				**			use_the_vertex_index_to_find_vertex_in_the_mesh's_array
				**		}
				**	}
				*/
				if( pMesh->m_aNormalArray   != NULL &&
					pMesh->m_aTexCoordArray != NULL )
				{
					unsigned int i;
					glBegin(GL_TRIANGLES);
					for(i=0;i<pMesh->m_iNumberOfFaces;i++)
					{
						unsigned int j;
						ObjFace *pf = &pMesh->m_aFaces[i];

						/*
						**	Draw the polygons with normals & uv co-ordinates
						*/
						for(j=0;j<3;j++)
						{
							glTexCoord2f( pMesh->m_aTexCoordArray[ pf->m_aTexCoordIndicies[j] ].u,
										  pMesh->m_aTexCoordArray[ pf->m_aTexCoordIndicies[j] ].v);
							glNormal3f( pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].x,
										pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].y,
										pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].z);
							glVertex3f( pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].x,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].y,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].z);
						}
					}
					glEnd();
				}
				else
				if( pMesh->m_aNormalArray   != NULL )
				{
					unsigned int i;
					glBegin(GL_TRIANGLES);
					for(i=0;i<pMesh->m_iNumberOfFaces;i++)
					{
						unsigned int j;
						ObjFace *pf = &pMesh->m_aFaces[i];

						/*
						**	Draw the polygons with Normals only
						*/
						for(j=0;j<3;j++)
						{
							glNormal3f( pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].x,
										pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].y,
										pMesh->m_aNormalArray[ pf->m_aNormalIndices[j] ].z);
							glVertex3f( pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].x,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].y,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].z);
						}
					}
					glEnd();
				}
				else
				if( pMesh->m_aTexCoordArray != NULL )
				{
					unsigned int i;
					glBegin(GL_TRIANGLES);
					for(i=0;i<pMesh->m_iNumberOfFaces;i++)
					{
						unsigned int j;
						ObjFace *pf = &pMesh->m_aFaces[i];

						/*
						**	Draw the polygons with Texturing co-ordinates only
						*/
						for(j=0;j<3;j++)
						{
							glTexCoord2f( pMesh->m_aTexCoordArray[ pf->m_aTexCoordIndicies[j] ].u,
										  pMesh->m_aTexCoordArray[ pf->m_aTexCoordIndicies[j] ].v);
							glVertex3f( pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].x,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].y,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].z);
						}
					}
					glEnd();
				}
				else
				{
					unsigned int i;
					glBegin(GL_TRIANGLES);
					for(i=0;i<pMesh->m_iNumberOfFaces;i++)
					{
						unsigned int j;
						ObjFace *pf = &pMesh->m_aFaces[i];

						/*
						**	Draw the polygons with Texturing co-ordinates only
						*/
						for(j=0;j<3;j++)
						{
							glVertex3f( pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].x,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].y,
										pMesh->m_aVertexArray[ pf->m_aVertexIndices[j] ].z);
						}
					}
					glEnd();
				}
			break;

			/*
			 *	Display lists are nice and easy to render...
			 */
			case DISPLAY_LIST:
				{
					glCallList(pMesh->m_iDisplayListNum);
				}
				break;

			/*
			 *	Render using vertex arrays
			 */
			case VERTEX_ARRAY:
				{
					/*
					**	Enable the vertex array
					*/
					glVertexPointer(3,GL_FLOAT,0,pMesh->m_aVertexArray);
					glEnableClientState(GL_VERTEX_ARRAY);

					/*
					**	Enable the normal array (if present)
					*/
					if(pMesh->m_aNormalArray)
					{
						glNormalPointer(GL_FLOAT,0,pMesh->m_aNormalArray);
						glEnableClientState(GL_NORMAL_ARRAY);
					}

					/*
					**	Enable the tex coord array (if present)
					*/
					if(pMesh->m_aTexCoordArray)
					{
						glTexCoordPointer(2,GL_FLOAT,0,pMesh->m_aTexCoordArray);
						glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					}

					/*
					**	Draw the vertex array data
					*/
					glDrawElements(GL_TRIANGLES,pMesh->m_iNumberOfFaces,GL_UNSIGNED_INT,pMesh->m_aIndices);

					/*
					**	Disable the arrays before exiting
					*/
					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				}
				break;

			/*
			 *	Render using vertex buffer objects
			 */
			case VERTEX_BUFFER_OBJECT:
				{

					// UNIT 0 
					// find dot product of N (stored in the texture map) and L (stored
					// as the PRIMARY_COLOR).
					glActiveTextureARB(GL_TEXTURE0_ARB);
					glBindTexture (GL_TEXTURE_2D, pMesh->m_BumpTexture);
					glEnable(GL_TEXTURE_2D);
					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
					glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_DOT3_RGB_EXT); 

					glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE);
					glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
					glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT);
					glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);

					// UNIT 1
					// modulate the base texture by N.L

					glActiveTextureARB(GL_TEXTURE1_ARB);
					glBindTexture (GL_TEXTURE_2D, pMesh->m_BaseTexture);
					glEnable(GL_TEXTURE_2D);

					glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
					glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);

					glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_PREVIOUS_EXT);
					glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_EXT, GL_SRC_COLOR);
					glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_TEXTURE);
					glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_EXT, GL_SRC_COLOR);

					glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->m_iVBO_Dynamic );

					/* load the seperate vertex, normal, tex coord, tangent & bi-normal arrays */
					glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0, 3 * pMesh->m_iVBO_Size * sizeof(float), pMesh->m_aVertexArray );
					glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 3 * pMesh->m_iVBO_Size * sizeof(float), 3 * pMesh->m_iVBO_Size * sizeof(float), pMesh->m_aNormalArray );
					glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 6 * pMesh->m_iVBO_Size * sizeof(float), 3 * pMesh->m_iVBO_Size * sizeof(float), pMesh->m_aColourArray );

					/*
					**	Enable the vertex array
					*/
					glVertexPointer(3,GL_FLOAT,0,0);
					glEnableClientState(GL_VERTEX_ARRAY);


					/*
					**	Enable the normal array (if present)
					*/
					glNormalPointer(GL_FLOAT,0, (void*)( 3 * pMesh->m_iVBO_Size * sizeof(float) ) );
					glEnableClientState(GL_NORMAL_ARRAY);

					glColorPointer(3,GL_FLOAT,0, (void*)( 6 * pMesh->m_iVBO_Size * sizeof(float) ) );
					glEnableClientState(GL_COLOR_ARRAY);


					/*
					**	Enable the tex coord array (if present)
					*/
					glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->m_iVBONum );
					
					glClientActiveTextureARB(GL_TEXTURE0_ARB);
					glTexCoordPointer(2,GL_FLOAT,0, 0 );
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					glClientActiveTextureARB(GL_TEXTURE1_ARB);
					glTexCoordPointer(2,GL_FLOAT,0, 0 );
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);

					/*
					**	Draw the vertex array data
					*/
					glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->m_iVBO_ElemNum );
					glDrawElements(GL_TRIANGLES,pMesh->m_iNumberOfFaces,GL_UNSIGNED_INT,0);

					/*
					**	Disable the arrays before exiting
					*/
					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_NORMAL_ARRAY);
					glClientActiveTextureARB(GL_TEXTURE0_ARB);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisable(GL_TEXTURE_2D);
					glClientActiveTextureARB(GL_TEXTURE1_ARB);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisable(GL_TEXTURE_2D);


					glDisable(GL_COLOR_MATERIAL);
				}
				break;

			default:
				break;
			}


		}
	}
}

/*
**	Calling free() on NULL is VERY BAD in C, so make sure we
**	check all pointers before calling free.
*/
void DeleteMesh(ObjMesh* pMesh)
{
	/*
	**	If the pointer is valid
	*/
	if(pMesh)
	{
		/*	delete the face array */
		if(pMesh->m_aFaces)
		{
			free(pMesh->m_aFaces);
			pMesh->m_aFaces = NULL;
		}

		/*	delete the vertex array */
		if(pMesh->m_aVertexArray)
		{
			free(pMesh->m_aVertexArray);
			pMesh->m_aVertexArray = NULL;
		}

		/*	delete the normal array */
		if(pMesh->m_aNormalArray)
		{
			free(pMesh->m_aNormalArray);
			pMesh->m_aNormalArray = NULL;
		}

		/*	delete the texturing co-ordinate array */
		if(pMesh->m_aTexCoordArray)
		{
			free(pMesh->m_aTexCoordArray);
			pMesh->m_aTexCoordArray = NULL;
		}

		/*	delete the texturing co-ordinate array */
		if(pMesh->m_aIndices)
		{
			free(pMesh->m_aIndices);
			pMesh->m_aIndices = NULL;
		}

		/*	delete the display list number	*/
		if(pMesh->m_iMode == DISPLAY_LIST)
		{
			glDeleteLists(pMesh->m_iDisplayListNum,1);
		}

		/*	free the mesh */
		free( pMesh );
	}
}

/*
**	This function just removes the specified object from the linked list
*/
void DeleteOBJ(ObjFile id)
{
	/*
	**	Create two pointers to walk through the linked list
	*/
	ObjMesh *pCurr,
			*pPrev = NULL;

	/*
	**	Start traversing the list from the start
	*/
	pCurr = g_LinkedListHead;

	/*
	**	Walk through the list until we either reach the end, or
	**	we find the node we are looking for
	*/
	while(pCurr != NULL && pCurr->m_iMeshID != id)
	{
		pPrev = pCurr;
		pCurr = pCurr->m_pNext;
	}

	/*
	**	If we found the node that needs to be deleted
	*/
	if(pCurr != NULL)
	{
		/*
		**	If the pointer before it is NULL, then we need to
		**	remove the first node in the list
		*/
		if(pPrev == NULL)
		{
			g_LinkedListHead = pCurr->m_pNext;
		}

		/*
		**	Otherwise we are removing a node from somewhere else
		*/
		else
		{
			pPrev->m_pNext = pCurr->m_pNext;
		}

		/*
		**	Free the memory allocated for this mesh
		*/
		DeleteMesh(pCurr);
	}
}

/*
**	This function just removes the specified object from the linked list
*/
void SetLightPosition(float lx,float ly,float lz)
{
	/*
	**	Create two pointers to walk through the linked list
	*/
	ObjMesh *pMesh = g_LinkedListHead;

	/*
	**	Walk through the list until we either reach the end, or
	**	we find the node we are looking for
	*/
	while(pMesh != NULL)
	{
		ObjVertex*  v = pMesh->m_aVertexArray;
		ObjVertex*  e = v + pMesh->m_iVBO_Size;
		ObjNormal*  n = pMesh->m_aNormalArray;
		ObjNormal*  b = pMesh->m_aBiNormalArray;
		ObjTangent* t = pMesh->m_aTangentArray;
		ObjColour*  c = pMesh->m_aColourArray;


		CalculateNormalArray( pMesh );
		CalculateTangentArray( pMesh );
		CalculateBiNormalArray( pMesh );

		
		for( ; v != e; ++v, ++b, ++n, ++t, ++c )
		{
			/* calculate light vector at the vertex */
			float lvx = lx - v->x;
			float lvy = ly - v->y;
			float lvz = lz - v->z;

			float it = 1.0f/((float)sqrt( lvx*lvx + lvy*lvy + lvz*lvz ));

			lvx *= it;
			lvy *= it;
			lvz *= it;

			c->r = lvx*t->x;
			c->g = lvx*t->y;
			c->b = lvx*t->z;

			c->r += lvy*b->x;
			c->g += lvy*b->y;
			c->b += lvy*b->z;

			c->r += lvz*n->x;
			c->g += lvz*n->y;
			c->b += lvz*n->z;

			c->r *= 0.5f;
			c->g *= 0.5f;
			c->b *= 0.5f;

			c->r += 0.5f;
			c->g += 0.5f;
			c->b += 0.5f;
		}


		pMesh = pMesh->m_pNext;
	}
}


/*
**	Delete all of the meshes starting from the front.
*/
void CleanUpOBJ(void)
{
	ObjMesh *pCurr;
	while(g_LinkedListHead != NULL)
	{
		pCurr = g_LinkedListHead;
		g_LinkedListHead = g_LinkedListHead->m_pNext;
		DeleteMesh(pCurr);
	}
}

unsigned int MakeGlTexture( unsigned char pData[],int iW,int iH,int bpp)
{

	/* The texture object - or, a number to identify the texture */
	unsigned int iTexObj;

	/* The Bit Depth as a GL constant */
	GLenum Depth;

	switch(bpp) {

	case 32: case 4: Depth = GL_RGBA; break;
	case 24: case 3: Depth = GL_RGB; break;
	case 8: case 1: Depth = GL_ALPHA; break;
	default: printf("Unsupported mode\n"); return 0;

	};

	/* generate a texture object number */
	glGenTextures(1,&iTexObj);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

	/* set the current texture to the one just created */
	glBindTexture (GL_TEXTURE_2D, iTexObj);

	/* repeat the texture in both directions */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	/* use bi-linear filtering to smooth the texture nicely. */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	 

	/* load the texture data into the graphics hardware. */
	glTexImage2D (GL_TEXTURE_2D,0,Depth,iW,iH,0,Depth,GL_UNSIGNED_BYTE,pData);

	return iTexObj;
 

}


void SetTextures(ObjFile id,const char BumpTex[],const char BaseTex[])
{
	/*
	**	Because the meshes are on a linked list, we first need to find the
	**	mesh with the specified ID number so traverse the list.
	*/
	ObjMesh *pMesh = g_LinkedListHead;
	unsigned int w,h,bpp;
	unsigned char* pixels=NULL;

	while(pMesh && pMesh->m_iMeshID != id)
	{
		pMesh = pMesh->m_pNext;
	}

	if(!pMesh)
		return;

	if (LoadTgaImage(BumpTex,&pixels,&w,&h,&bpp)) {
		pMesh->m_BumpTexture = MakeGlTexture(pixels,w,h,bpp);
		free(pixels);
	}
	if (LoadTgaImage(BaseTex,&pixels,&w,&h,&bpp)) {
		pMesh->m_BaseTexture = MakeGlTexture(pixels,w,h,bpp);
		free(pixels);
	}
}

/*
**	This function converts the specified mesh into either a display list,
**	vertex array, or vertex buffer object.
*/
void ConvertMeshTo(ObjFile id,unsigned char What)
{
	/*
	**	Because the meshes are on a linked list, we first need to find the
	**	mesh with the specified ID number so traverse the list.
	*/
	ObjMesh *pMesh = g_LinkedListHead;

	while(pMesh && pMesh->m_iMeshID != id)
	{
		pMesh = pMesh->m_pNext;
	}

	/*
	**	If the mesh has already been converted to a display list or a vertex array, return.
	*/
	if(pMesh->m_iMode != 0)
		return;

	switch(What)
	{
	case DISPLAY_LIST:
		{
			/*
			**	Generate a display list number
			*/
			pMesh->m_iDisplayListNum = glGenLists(1);

			/*
			**	Make the display list to draw the object
			*/
			glNewList(pMesh->m_iDisplayListNum,GL_COMPILE);
				DrawOBJ(id);
			glEndList();

			/*
			**	Assign the new mode value to the mesh
			*/
			pMesh->m_iMode = What;

			/*
			**	Delete all the data held on the node and set values to NULL
			*/
			if(pMesh->m_aFaces)
				free(pMesh->m_aFaces);
			pMesh->m_aFaces = NULL;

			if(pMesh->m_aVertexArray)
				free(pMesh->m_aVertexArray);
			pMesh->m_aVertexArray = NULL;

			if(pMesh->m_aNormalArray)
				free(pMesh->m_aNormalArray);
			pMesh->m_aNormalArray = NULL;

			if(pMesh->m_aTexCoordArray)
				free(pMesh->m_aTexCoordArray);
			pMesh->m_aTexCoordArray = NULL;

			/*
			**	Set counts to zero
			*/
			pMesh->m_iNumberOfFaces		= 0;
			pMesh->m_iNumberOfNormals	= 0;
			pMesh->m_iNumberOfTexCoords = 0;
			pMesh->m_iNumberOfVertices	= 0;

		}
		break;

	case VERTEX_BUFFER_OBJECT:
	case VERTEX_ARRAY:
		{
			unsigned int i,j,k,l,m=0;

			/*
			**	Create a temporary set of pointers to the old data....
			*/
			ObjVertex   *pVT = pMesh->m_aVertexArray;
			ObjNormal   *pNT = pMesh->m_aNormalArray;
			ObjTexCoord *pTT = pMesh->m_aTexCoordArray;

			/*
			**	This is used to keep track of the actual vertex array size created.
			**	Once the data has been organised into 
			*/
			unsigned int ActualVertexArraySize=0;

			InitOpenGL_Extensions();

			/*
			**	Allocate a new set of arrays to hold the expanded data
			*/
			pMesh->m_aVertexArray		= (ObjVertex*)  malloc(pMesh->m_iNumberOfFaces*3*sizeof(ObjVertex));
			assert( pMesh->m_aVertexArray );

			if(pMesh->m_aNormalArray)
			{
				pMesh->m_aNormalArray   = (ObjNormal*)  malloc(pMesh->m_iNumberOfFaces*3*sizeof(ObjNormal));
				assert(pMesh->m_aNormalArray);
			}
			
			if(pMesh->m_aTexCoordArray)
			{
				pMesh->m_aTexCoordArray = (ObjTexCoord*)malloc(pMesh->m_iNumberOfFaces*3*sizeof(ObjTexCoord));
				assert(pMesh->m_aTexCoordArray);
			}


			/* 
			**	Allocate memory for the vertex array indices
			*/
			pMesh->m_aIndices	= (unsigned int*)malloc( pMesh->m_iNumberOfFaces*3*sizeof(unsigned int)	  );

			assert(pMesh->m_aVertexArray);

			/*
			**	write each vertex/normal/and uv co-ordinate out to the new array
			**	so that we can use vertex arrays....
			*/
			for(i=0,j=0;i!=pMesh->m_iNumberOfFaces;i++)
			{
				for(k=0;k!=3;k++,j++)
				{
					float v[3]={0},n[3]={0},t[2]={0};

					#define EPISILON 0.0001f
					#define FCOMP(A,B) (((A)>((B)-EPISILON)) && ((A)<((B)+EPISILON)))

					v[0] = pVT[ pMesh->m_aFaces[i].m_aVertexIndices[k] ].x;
					v[1] = pVT[ pMesh->m_aFaces[i].m_aVertexIndices[k] ].y;
					v[2] = pVT[ pMesh->m_aFaces[i].m_aVertexIndices[k] ].z;

					if(pMesh->m_aNormalArray)
					{
						n[0]   = pNT[ pMesh->m_aFaces[i].m_aNormalIndices[k] ].x;
						n[1]   = pNT[ pMesh->m_aFaces[i].m_aNormalIndices[k] ].y;
						n[2]   = pNT[ pMesh->m_aFaces[i].m_aNormalIndices[k] ].z;
					}

					if(pMesh->m_aTexCoordArray)
					{
						t[0]   = pTT[ pMesh->m_aFaces[i].m_aTexCoordIndicies[k] ].u;
						t[1]   = pTT[ pMesh->m_aFaces[i].m_aTexCoordIndicies[k] ].v;
					}
					
					for(l=0;l!=ActualVertexArraySize;++l) 
					{
                        if( FCOMP( pMesh->m_aVertexArray[l].x, v[0] ) && 
							FCOMP( pMesh->m_aVertexArray[l].y, v[1] ) &&
							FCOMP( pMesh->m_aVertexArray[l].z, v[2] ) ) {

							if(pMesh->m_aNormalArray)
							{
								if( FCOMP( pMesh->m_aNormalArray[l].x, n[0] ) && 
									FCOMP( pMesh->m_aNormalArray[l].y, n[1] ) &&
									FCOMP( pMesh->m_aNormalArray[l].z, n[2] ) )
								{
									if(pMesh->m_aTexCoordArray)
									{
										if( FCOMP( pMesh->m_aTexCoordArray[l].u, t[0] ) && 
											FCOMP( pMesh->m_aTexCoordArray[l].v, t[1] )  )
										{
											goto reuse_element;
										}
									}
									else 
										goto reuse_element;
								}
							}
							else
							if(pMesh->m_aTexCoordArray)
							{
								if( FCOMP( pMesh->m_aTexCoordArray[l].u, t[0] ) && 
									FCOMP( pMesh->m_aTexCoordArray[l].v, t[1] )  )
								{
									goto reuse_element;
								}
							}
							else
								goto reuse_element;
                        }
					}

					pMesh->m_aVertexArray[ActualVertexArraySize].x		 = v[0];
					pMesh->m_aVertexArray[ActualVertexArraySize].y		 = v[1];
					pMesh->m_aVertexArray[ActualVertexArraySize].z		 = v[2];

					if(pMesh->m_aNormalArray)
					{
						pMesh->m_aNormalArray[ActualVertexArraySize].x   = n[0];
						pMesh->m_aNormalArray[ActualVertexArraySize].y   = n[1];
						pMesh->m_aNormalArray[ActualVertexArraySize].z   = n[2];
					}

					if(pMesh->m_aTexCoordArray)
					{
						pMesh->m_aTexCoordArray[ActualVertexArraySize].u = t[0];
						pMesh->m_aTexCoordArray[ActualVertexArraySize].v = t[1];
					}

					pMesh->m_aIndices[m] = ActualVertexArraySize++;

					goto end_elem;

					reuse_element:
						pMesh->m_aIndices[m] = l;

					end_elem:
						++m;
				}
			}
			
			pMesh->m_aVertexArray		= (ObjVertex*)  realloc(pMesh->m_aVertexArray,ActualVertexArraySize*sizeof(ObjVertex));
			assert( pMesh->m_aVertexArray );

			if(pMesh->m_aNormalArray)
			{
				pMesh->m_aNormalArray   = (ObjNormal*)  realloc(pMesh->m_aNormalArray,ActualVertexArraySize*sizeof(ObjNormal));
				assert(pMesh->m_aNormalArray);
			}
			
			if(pMesh->m_aTexCoordArray)
			{
				pMesh->m_aTexCoordArray = (ObjTexCoord*)realloc(pMesh->m_aTexCoordArray,ActualVertexArraySize*sizeof(ObjTexCoord));
				assert(pMesh->m_aTexCoordArray);
			}

			/*
			**	Using the number of faces variable as the count of all the vertices in the array
			*/
			pMesh->m_iNumberOfFaces*=3;

			/*
			**	Zero the data counts
			*/
			pMesh->m_iNumberOfNormals	= 0;
			pMesh->m_iNumberOfTexCoords = 0;
			pMesh->m_iNumberOfVertices	= 0;

			/*
			**	Free the old data....
			*/
			if(pVT)
				free(pVT);
			if(pNT)
				free(pNT);
			if(pTT)
				free(pTT);
			if(pMesh->m_aFaces)
				free(pMesh->m_aFaces);
			pMesh->m_aFaces = NULL;

			pMesh->m_iVBO_Size = ActualVertexArraySize;

			pMesh->m_aTangentArray = (ObjTangent*) malloc( ActualVertexArraySize*sizeof(ObjTangent) );
			assert(pMesh->m_aTangentArray);

			pMesh->m_aBiNormalArray   = (ObjNormal*)malloc(ActualVertexArraySize*sizeof(ObjNormal));
			assert(pMesh->m_aBiNormalArray);

			pMesh->m_aColourArray   = (ObjColour*)malloc(ActualVertexArraySize*sizeof(ObjColour));
			assert(pMesh->m_aColourArray);


			CalculateNormalArray( pMesh );
			CalculateTangentArray( pMesh );
			CalculateBiNormalArray( pMesh );

			if(What == VERTEX_ARRAY)
			{
				/*
				**	Set the mode to vertex array
				*/
				pMesh->m_iMode = VERTEX_ARRAY;
			}
			else 
			{
				pMesh->m_iMode = VERTEX_BUFFER_OBJECT;

				/* generate 2 buffers for the vertex buffer objects */
				glGenBuffersARB( 1, &pMesh->m_iVBONum ); 
				glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->m_iVBONum );
				glBufferDataARB( GL_ARRAY_BUFFER_ARB, 2 * ActualVertexArraySize * sizeof(float), pMesh->m_aTexCoordArray, GL_STATIC_DRAW_ARB );


				/* generate a vertex buffer object */
				glGenBuffersARB( 1, &pMesh->m_iVBO_ElemNum );		
				glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->m_iVBO_ElemNum );
				glBufferDataARB( GL_ELEMENT_ARRAY_BUFFER_ARB, pMesh->m_iNumberOfFaces * sizeof(float), pMesh->m_aIndices, GL_DYNAMIC_DRAW_ARB );


				glGenBuffersARB( 1, &pMesh->m_iVBO_Dynamic ); 
				glBindBufferARB( GL_ARRAY_BUFFER_ARB, pMesh->m_iVBO_Dynamic );

				/* set vertex buffer object size */
				glBufferDataARB( GL_ARRAY_BUFFER_ARB, 9 * ActualVertexArraySize * sizeof(float), NULL, GL_STATIC_DRAW_ARB );

				/* load the seperate vertex, normal, tex coord, tangent & bi-normal arrays */
				glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 0, 3 * ActualVertexArraySize * sizeof(float), pMesh->m_aVertexArray );
				glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 3 * ActualVertexArraySize * sizeof(float), 3 * ActualVertexArraySize * sizeof(float), pMesh->m_aNormalArray );
				glBufferSubDataARB( GL_ARRAY_BUFFER_ARB, 6 * ActualVertexArraySize * sizeof(float), 3 * ActualVertexArraySize * sizeof(float), pMesh->m_aColourArray );



				/*
				**	Because all of the data has now been uploaded to the graphics card, we
				**	can simply delete all of the un-used client side data. joy!
				*/


			}
		}
		break;

	default:
		break;
	}
}
