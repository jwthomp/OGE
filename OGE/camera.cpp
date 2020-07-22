#include "camera.h"	

#include "glew/glew.h"

camera::camera()
{
	m_transform.set_identity();
}

camera::~camera()
{
}

void camera::set_perspective() const
{
	Vector3 fvec = m_transform.get_fvec();
	Vector3 uvec = m_transform.get_uvec();

	gluLookAt(m_transform._03, m_transform._13, m_transform._23,	
				m_transform._03 + fvec.m_data[0], m_transform._13 + fvec.m_data[1], m_transform._23 + fvec.m_data[2], 
				uvec.m_data[0], uvec.m_data[1], uvec.m_data[2]);
}

void camera::set_transform(matrix44 const* p_transform)
{
	m_transform = *p_transform;
}

matrix44 const* camera::get_transform() const
{
	return &m_transform;
}