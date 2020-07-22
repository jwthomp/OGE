#include "matrix.h"

#include "assert.h"

#include <string.h>

Vector3 matrix44::get_fvec() const
{
	Vector3 tmp_vec;
	tmp_vec.m_data[0] = _02;
	tmp_vec.m_data[1] = _12;
	tmp_vec.m_data[2] = _22;

	return tmp_vec;
}

Vector3 matrix44::get_uvec() const
{
	Vector3 tmp_vec;
	tmp_vec.m_data[0] = _01;
	tmp_vec.m_data[1] = _11;
	tmp_vec.m_data[2] = _21;

	return tmp_vec;
}

Vector3 matrix44::get_rvec() const
{
	Vector3 tmp_vec;
	tmp_vec.m_data[0] = _00;
	tmp_vec.m_data[1] = _10;
	tmp_vec.m_data[2] = _20;

	return tmp_vec;
}

Vector3 matrix44::get_trans() const
{
	Vector3 tmp_vec;
	tmp_vec.m_data[0] = _03;
	tmp_vec.m_data[1] = _13;
	tmp_vec.m_data[2] = _23;

	return tmp_vec;
}

void matrix44::set_identity()
{
	assert(sizeof(m_data) == 64);
	memset(m_data, 0, sizeof(m_data));

	m_data[0] = 1.0f;
	m_data[5] = 1.0f;
	m_data[10] = 1.0f;
	m_data[15] = 1.0f;
}

Vector3 matrix44::operator*(Vector3 const& val) const
{
	Vector3 out;

	//out.m_data[0] = (val.m_data[0] * _00) + (val.m_data[1] * _10) + (val.m_data[2] * _20) + _30;
	//out.m_data[1] = (val.m_data[0] * _01) + (val.m_data[1] * _11) + (val.m_data[2] * _21) + _31;
	//out.m_data[2] = (val.m_data[0] * _02) + (val.m_data[1] * _12) + (val.m_data[2] * _22) + _32;

	out.m_data[0] = (val.m_data[0] * _00) + (val.m_data[1] * _01) + (val.m_data[2] * _02) + _03;
	out.m_data[1] = (val.m_data[0] * _10) + (val.m_data[1] * _11) + (val.m_data[2] * _12) + _13;
	out.m_data[2] = (val.m_data[0] * _20) + (val.m_data[1] * _21) + (val.m_data[2] * _22) + _23;

	return out;
}

matrix44 matrix44::operator*(matrix44 const& _m) const
{
	matrix44 result;

#if 1
	result._00 = (_00 * _m._00) + (_10 * _m._01) + (_20 * _m._02) + (_30 * _m._03);
	result._01 = (_01 * _m._00) + (_11 * _m._01) + (_21 * _m._02) + (_31 * _m._03);
	result._02 = (_02 * _m._00) + (_12 * _m._01) + (_22 * _m._02) + (_32 * _m._03);
	result._03 = (_03 * _m._00) + (_13 * _m._01) + (_23 * _m._02) + (_33 * _m._03);
	
	result._10 = (_00 * _m._10) + (_10 * _m._11) + (_20 * _m._12) + (_30 * _m._13);
	result._11 = (_01 * _m._10) + (_11 * _m._11) + (_21 * _m._12) + (_31 * _m._13);
	result._12 = (_02 * _m._10) + (_12 * _m._11) + (_22 * _m._12) + (_32 * _m._13);
	result._13 = (_03 * _m._10) + (_13 * _m._11) + (_23 * _m._12) + (_33 * _m._13);
	
	result._20 = (_00 * _m._20) + (_10 * _m._21) + (_20 * _m._22) + (_30 * _m._23);
	result._21 = (_01 * _m._20) + (_11 * _m._21) + (_21 * _m._22) + (_31 * _m._23);
	result._22 = (_02 * _m._20) + (_12 * _m._21) + (_22 * _m._22) + (_32 * _m._23);
	result._23 = (_03 * _m._20) + (_13 * _m._21) + (_23 * _m._22) + (_33 * _m._23);
	
	result._30 = (_00 * _m._30) + (_10 * _m._31) + (_20 * _m._32) + (_30 * _m._33);
	result._31 = (_01 * _m._30) + (_11 * _m._31) + (_21 * _m._32) + (_31 * _m._33);
	result._32 = (_02 * _m._30) + (_12 * _m._31) + (_22 * _m._32) + (_32 * _m._33);
	result._33 = (_03 * _m._30) + (_13 * _m._31) + (_23 * _m._32) + (_33 * _m._33);

#else
	result._00 = (_00 * _m._00) + (_01 * _m._10) + (_02 * _m._20) + (_03 * _m._30);
	result._01 = (_00 * _m._01) + (_01 * _m._11) + (_02 * _m._21) + (_03 * _m._31);
	result._02 = (_00 * _m._02) + (_01 * _m._12) + (_02 * _m._22) + (_03 * _m._32);
	result._03 = (_00 * _m._03) + (_01 * _m._13) + (_02 * _m._23) + (_03 * _m._33);
	
	result._10 = (_10 * _m._00) + (_11 * _m._10) + (_12 * _m._20) + (_13 * _m._30);
	result._11 = (_10 * _m._01) + (_11 * _m._11) + (_12 * _m._21) + (_13 * _m._31);
	result._12 = (_10 * _m._02) + (_11 * _m._12) + (_12 * _m._22) + (_13 * _m._32);
	result._13 = (_10 * _m._03) + (_11 * _m._13) + (_12 * _m._23) + (_13 * _m._33);
	
	result._20 = (_20 * _m._00) + (_21 * _m._10) + (_22 * _m._20) + (_23 * _m._30);
	result._21 = (_20 * _m._01) + (_21 * _m._11) + (_22 * _m._21) + (_23 * _m._31);
	result._22 = (_20 * _m._02) + (_21 * _m._12) + (_22 * _m._22) + (_23 * _m._32);
	result._23 = (_20 * _m._03) + (_21 * _m._13) + (_22 * _m._23) + (_23 * _m._33);
	
	result._30 = (_30 * _m._00) + (_31 * _m._10) + (_32 * _m._20) + (_33 * _m._30);
	result._31 = (_30 * _m._01) + (_31 * _m._11) + (_32 * _m._21) + (_33 * _m._31);
	result._32 = (_30 * _m._02) + (_31 * _m._12) + (_32 * _m._22) + (_33 * _m._32);
	result._33 = (_30 * _m._03) + (_31 * _m._13) + (_32 * _m._23) + (_33 * _m._33);
#endif

	return result;
}

matrix44 matrix44::inverse() const
{
	real fA0 = m_data[ 0]*m_data[ 5] - m_data[ 1]*m_data[ 4];
	real fA1 = m_data[ 0]*m_data[ 6] - m_data[ 2]*m_data[ 4];
	real fA2 = m_data[ 0]*m_data[ 7] - m_data[ 3]*m_data[ 4];
	real fA3 = m_data[ 1]*m_data[ 6] - m_data[ 2]*m_data[ 5];
	real fA4 = m_data[ 1]*m_data[ 7] - m_data[ 3]*m_data[ 5];
	real fA5 = m_data[ 2]*m_data[ 7] - m_data[ 3]*m_data[ 6];
	real fB0 = m_data[ 8]*m_data[13] - m_data[ 9]*m_data[12];
	real fB1 = m_data[ 8]*m_data[14] - m_data[10]*m_data[12];
	real fB2 = m_data[ 8]*m_data[15] - m_data[11]*m_data[12];
	real fB3 = m_data[ 9]*m_data[14] - m_data[10]*m_data[13];
	real fB4 = m_data[ 9]*m_data[15] - m_data[11]*m_data[13];
	real fB5 = m_data[10]*m_data[15] - m_data[11]*m_data[14];

	real fDet = fA0*fB5-fA1*fB4+fA2*fB3+fA3*fB2-fA4*fB1+fA5*fB0;
	//if (Math<Real>::FAbs(fDet) <= Math<Real>::ZERO_TOLERANCE) {
   //     return Matrix4<Real>::ZERO;
   // }

	matrix44 kInv;
	kInv.m_data[ 0] =	+ m_data[ 5]*fB5 - m_data[ 6]*fB4 + m_data[ 7]*fB3;
	kInv.m_data[ 4] =	- m_data[ 4]*fB5 + m_data[ 6]*fB2 - m_data[ 7]*fB1;
	kInv.m_data[ 8] = + m_data[ 4]*fB4 - m_data[ 5]*fB2 + m_data[ 7]*fB0;
	kInv.m_data[12] =	- m_data[ 4]*fB3 + m_data[ 5]*fB1 - m_data[ 6]*fB0;
	kInv.m_data[ 1] =	- m_data[ 1]*fB5 + m_data[ 2]*fB4 - m_data[ 3]*fB3;
	kInv.m_data[ 5] =	+ m_data[ 0]*fB5 - m_data[ 2]*fB2 + m_data[ 3]*fB1;
	kInv.m_data[ 9] = - m_data[ 0]*fB4 + m_data[ 1]*fB2 - m_data[ 3]*fB0;
	kInv.m_data[13] =	+ m_data[ 0]*fB3 - m_data[ 1]*fB1 + m_data[ 2]*fB0;
	kInv.m_data[ 2] =	+ m_data[13]*fA5 - m_data[14]*fA4 + m_data[15]*fA3;
	kInv.m_data[ 6] =	- m_data[12]*fA5 + m_data[14]*fA2 - m_data[15]*fA1;
	kInv.m_data[10] =	+ m_data[12]*fA4 - m_data[13]*fA2 + m_data[15]*fA0;
	kInv.m_data[14] =	- m_data[12]*fA3 + m_data[13]*fA1 - m_data[14]*fA0;
	kInv.m_data[ 3] =	- m_data[ 9]*fA5 + m_data[10]*fA4 - m_data[11]*fA3;
	kInv.m_data[ 7] =	+ m_data[ 8]*fA5 - m_data[10]*fA2 + m_data[11]*fA1;
	kInv.m_data[11] =	- m_data[ 8]*fA4 + m_data[ 9]*fA2 - m_data[11]*fA0;
	kInv.m_data[15] =	+ m_data[ 8]*fA3 - m_data[ 9]*fA1 + m_data[10]*fA0;

	real fInvDet = ((real)1.0)/fDet;
	kInv.m_data[ 0] *= fInvDet;
	kInv.m_data[ 1] *= fInvDet;
	kInv.m_data[ 2] *= fInvDet;
	kInv.m_data[ 3] *= fInvDet;
	kInv.m_data[ 4] *= fInvDet;
	kInv.m_data[ 5] *= fInvDet;
	kInv.m_data[ 6] *= fInvDet;
	kInv.m_data[ 7] *= fInvDet;
	kInv.m_data[ 8] *= fInvDet;
	kInv.m_data[ 9] *= fInvDet;
	kInv.m_data[10] *= fInvDet;
	kInv.m_data[11] *= fInvDet;
	kInv.m_data[12] *= fInvDet;
	kInv.m_data[13] *= fInvDet;
	kInv.m_data[14] *= fInvDet;
	kInv.m_data[15] *= fInvDet;

	return kInv;
}

void matrix44::set_translation(Vector3 const& p_val)
{
	_03 = p_val.m_data[0];
	_13 = p_val.m_data[1];
	_23 = p_val.m_data[2];
}