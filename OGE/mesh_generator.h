#ifndef __MESH_GENERATOR_H_
#define __MESH_GENERATOR_H_

class mesh;
class Vector3;

mesh *mesh_generator_from_triangles(Vector3 const p_vectors[][3], unsigned long p_triangle_count);

#endif // __MESH_GENERATOR_H_