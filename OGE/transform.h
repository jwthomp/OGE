#ifndef __TRANSFORM_H_
#define __TRANSFORM_H_

#include "vector3.h"
#include "quaternion.h"
#include "matrix.h"

class transform
{
public:
	transform();

	Vector3 const& get_position();
	Vector3 const& get_scale();
	quaternion const& get_orient();

	void set_values(Vector3 const* p_position, quaternion const* p_orient, Vector3 const* p_scale);

	matrix44 m_transform_matrix;

private:
	Vector3 m_position;
	quaternion m_orientation;
	Vector3 m_scale;

};

#endif /* __TRANSFORM_H_ */