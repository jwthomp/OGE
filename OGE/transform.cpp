#include "transform.h"	

transform::transform()
{
	m_transform_matrix.set_identity();
	m_position.set(0.0f, 0.0f, 0.0f);
	m_orientation.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, 0.0f);
}


Vector3 const& transform::get_position()
{
	return m_position;
}

Vector3 const& transform::get_scale()
{
	return m_scale;
}

quaternion const& transform::get_orient()
{
	return m_orientation;
}

	
void transform::set_values(Vector3 const* p_position, quaternion const* p_orient, Vector3 const* p_scale)
{
	if (p_position) {
		m_position = *p_position;
	}

	if (p_orient) {
		m_orientation = *p_orient;
	}

	if (p_scale) {
		m_scale = *p_scale;
	}

	m_orientation.CreateMatrix(m_transform_matrix.m_data);

	

	matrix44 tmp_scale;
	tmp_scale.set_identity();
	tmp_scale._00 *= m_scale.m_data[0];
	tmp_scale._11 *= m_scale.m_data[1];
	tmp_scale._22 *= m_scale.m_data[2];
	tmp_scale._33 = 1.0f;

	
	m_transform_matrix = tmp_scale * m_transform_matrix;

	m_transform_matrix._03 = m_position.m_data[0];
	m_transform_matrix._13 = m_position.m_data[1];
	m_transform_matrix._23 = m_position.m_data[2];
}

