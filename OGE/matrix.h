#ifndef __MATRIX_H_
#define __MATRIX_H_

#include "vector3.h"

class matrix44
{
public:
	union {
		real m_data[16];
		struct {
			float _00, _10, _20, _30;
			float _01, _11, _21, _31;
			float _02, _12, _22, _32;
			float _03, _13, _23, _33;

//			float _00, _01, _02, _03;
//			float _10, _11, _12, _13;
//			float _20, _21, _22, _23;
//			float _30, _31, _32, _33;
		};
		struct {
			Vector3 rvec; float a;
			Vector3 uvec; float b;
			Vector3 fvec; float c;
			Vector3 pvec; float d;
		};
	};

	Vector3 get_fvec() const;
	Vector3 get_uvec() const;
	Vector3 get_rvec() const;
	Vector3 get_trans() const;
	void set_identity();
	
	Vector3 operator*(Vector3 const& val) const;
	matrix44 operator*(matrix44 const& val) const;

	void set_translation(Vector3 const& p_val);

	float determinant() const;
	matrix44 inverse() const;
};

#endif // __MATRIX_H_

