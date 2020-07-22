#ifndef __QUATERNION_H_
#define __QUATERNION_H_

#include "core_types.h"

class quaternion
{
public:
	void CreateFromAxisAngle(real x, real y, real z, real degrees);
	void CreateFromAngles(real p_roll, real p_pitch, real p_yaw);

	void CreateMatrix(real *pMatrix) const;

	quaternion operator *(quaternion q) const;
	
	quaternion();
	~quaternion();

private:
	real m_w;
	real m_z;
	real m_y;
	real m_x;
};

#endif // __QUATERNION_H_
