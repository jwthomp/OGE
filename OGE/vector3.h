#ifndef __VECTOR3_H_
#define __VECTOR3_H_

#include <math.h>

#include "core_types.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

class Vector3 {
public:
	union {
		real m_data[3];
		struct {
			real x;
			real y;
			real z;
		};
	};

	Vector3();

	Vector3(float p_x, float p_y, float p_z);
	
	Vector3(Vector3 const& p_ref);
	
	Vector3 operator-(Vector3 const& a) const;

	Vector3 operator+(Vector3 const& a) const;

	Vector3 operator*(real val) const;

	Vector3 operator+(real val) const;

	real operator*(Vector3 const& a) const;

	Vector3& operator*=(real val);

	Vector3 operator/(real val) const;

	Vector3& operator+=( const Vector3& val );

	Vector3& operator-=( const Vector3& val );

	Vector3& operator=(const Vector3& val);

	void set(real x, real y, real z);

	Vector3 cross(Vector3 const& val);

	float len() const;

};

Vector3 vmax(Vector3 const& val_a, Vector3 const& val_b);

Vector3 vmin(Vector3 const& val_a, Vector3 const& val_b);

#endif // __VECTOR3_H_