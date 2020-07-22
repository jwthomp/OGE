#ifndef __RENDER_LIB_TYPES_H_
#define __RENDER_LIB_TYPES_H_

typedef unsigned long mesh_id;
#define RENDER_LIB_MESH_ID_INVALID ((mesh_id) 0xFFFFFFFF)

typedef unsigned char mesh_instance_type;
const mesh_instance_type RENDER_LIB_MESH_INSTANCE_TYPE_STATIC = 0;
const mesh_instance_type RENDER_LIB_MESH_INSTANCE_TYPE_DYNAMIC = 1;


typedef unsigned char mesh_format;
const mesh_format RENDER_LIB_MESH_FORMAT_VA_TRIANGLES = 0;
const mesh_format RENDER_LIB_MESH_FORMAT_VA_TRIANGLE_STRIP = 1;


#endif // __RENDER_LIB_TYPES_H_
