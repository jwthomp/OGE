#include "vector3.h"

Vector3::Vector3() {
	m_data[0] = 0.0f;
	m_data[1] = 0.0f;
	m_data[2] = 0.0f;
}

Vector3::Vector3(real p_x, real p_y, real p_z) {
	m_data[0] = p_x;
	m_data[1] = p_y;
	m_data[2] = p_z;
}

Vector3::Vector3(Vector3 const& p_ref) {
	m_data[0] = p_ref.m_data[0];
	m_data[1] = p_ref.m_data[1];
	m_data[2] = p_ref.m_data[2];
}

Vector3 Vector3::operator-(Vector3 const& a) const {
	Vector3 ret;
	ret.m_data[0] = m_data[0] - a.m_data[0];
	ret.m_data[1] = m_data[1] - a.m_data[1];
	ret.m_data[2] = m_data[2] - a.m_data[2];

	return ret;
}

Vector3 Vector3::operator+(Vector3 const& a) const {
	Vector3 ret;
	ret.m_data[0] = m_data[0] + a.m_data[0];
	ret.m_data[1] = m_data[1] + a.m_data[1];
	ret.m_data[2] = m_data[2] + a.m_data[2];
   return ret;
}

Vector3 Vector3::operator*(real val) const {
	Vector3 ret;
	ret.m_data[0] = m_data[0] * val;
	ret.m_data[1] = m_data[1] * val;
	ret.m_data[2] = m_data[2] * val;

	return ret;
}

Vector3 Vector3::operator+(real val) const {
	Vector3 ret;
	ret.m_data[0] = m_data[0] + val;
	ret.m_data[1] = m_data[1] + val;
	ret.m_data[2] = m_data[2] + val;

	return ret;
}

real Vector3::operator*(Vector3 const& a) const {
	return (m_data[0] * a.m_data[0]) + (m_data[1] * a.m_data[1]) + (m_data[2] * a.m_data[2]);
}

Vector3& Vector3::operator*=(real val) {
	m_data[0] *= val;
	m_data[1] *= val;
	m_data[2] *= val;

	return *this;
}

Vector3 Vector3::operator/(real val) const {
	Vector3 ret;
	ret.m_data[0] = m_data[0] / val;
	ret.m_data[1] = m_data[1] / val;
	ret.m_data[2] = m_data[2] / val;

	return ret;
}

Vector3& Vector3::operator+=( const Vector3& val ) {
	m_data[0] += val.m_data[0];
	m_data[1] += val.m_data[1];
	m_data[2] += val.m_data[2];

	return *this;
}

Vector3& Vector3::operator-=( const Vector3& val ) {
	m_data[0] -= val.m_data[0];
	m_data[1] -= val.m_data[1];
	m_data[2] -= val.m_data[2];

	return *this;
}

Vector3& Vector3::operator=(const Vector3& val) {
	m_data[0] = val.m_data[0];
	m_data[1] = val.m_data[1];
	m_data[2] = val.m_data[2];

	return *this;
}

void Vector3::set(real x, real y, real z) {
	m_data[0] = x;
	m_data[1] = y;
	m_data[2] = z;
}

float Vector3::len() const {
	return sqrt((m_data[0] * m_data[0]) + (m_data[1] * m_data[1]) + (m_data[2] * m_data[2]));
}

Vector3 Vector3::cross(Vector3 const& val)
{
	return Vector3((m_data[1] * val.m_data[2]) - (m_data[2] - val.m_data[1]),
						(m_data[2] * val.m_data[0]) - (m_data[0] * val.m_data[2]),
						(m_data[0] * val.m_data[1]) - (m_data[1] * val.m_data[0]));
}

Vector3 vmax(Vector3 const& val_a, Vector3 const& val_b) {
	return Vector3(max(val_a.m_data[0], val_b.m_data[0]),
		max(val_a.m_data[1], val_b.m_data[1]),
		max(val_a.m_data[2], val_b.m_data[2]));
}

Vector3 vmin(Vector3 const& val_a, Vector3 const& val_b) {
	return Vector3(min(val_a.m_data[0], val_b.m_data[0]),
		min(val_a.m_data[1], val_b.m_data[1]),
		min(val_a.m_data[2], val_b.m_data[2]));
}

