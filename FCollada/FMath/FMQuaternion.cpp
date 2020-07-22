/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FMath/FMath.h"
#include "FMath/FMVector3.h"
#include "FMath/FMQuaternion.h"

//
// FMQuaternion
//

const FMQuaternion FMQuaternion::Zero = FMQuaternion(0.0f, 0.0f, 0.0f, 0.0f);
const FMQuaternion FMQuaternion::Identity = FMQuaternion(0.0f, 0.0f, 0.0f, 1.0f);

FMQuaternion::FMQuaternion(const FMVector3& axis, float angle)
{
	float s = sinf(angle / 2.0f);
	x = axis.x * s;
	y = axis.y * s;
	z = axis.z * s;
	w = cosf(angle / 2.0f);
}

FMQuaternion FMQuaternion::operator*(const FMQuaternion& q) const
{
	FMQuaternion r;
	r.x = w * q.x + x * q.w + y * q.z - z * q.y;
	r.y = w * q.y + y * q.w + z * q.x - x * q.z;
	r.z = w * q.z + z * q.w + x * q.y - y * q.x;
	r.w = w * q.w - x * q.x - y * q.y - z * q.z;
	return r;
}

FMVector3 FMQuaternion::operator*(const FMVector3& v) const
{
	FMQuaternion out = (*this) * FMQuaternion(v.x, v.y, v.z, 0.0f) * (~(*this));
	return FMVector3(out.x, out.y, out.z);
}

void FMQuaternion::ToAngleAxis(FMVector3& axis, float& angle) const
{
	angle = 2.0f * acosf(w);
	float s = sinf(angle / 2.0f);
	if (!IsEquivalent(s, 0.0f))
	{
		axis.x = x / s;
		axis.y = y / s;
		axis.z = z / s;
		axis.NormalizeIt();
	}
	else
	{
		// If s == 0, then angle == 0 and there is no rotation: assign any axis.
		axis = FMVector3::XAxis;
	}
}

// [GLaforte - 31/08/2006: this is a work in progress...]
FMVector3 FMQuaternion::ToEuler(FMVector3* UNUSED(previousAngles)) const
{
	FMVector3 angles;

	// Convert the quaternion into Euler angles.
	float siny = 2.0f * (x * z + y * w);
	if (siny > 1.0f - FLT_TOLERANCE) // singularity at north pole
	{ 
		angles.y = (float) FMath::Pi / 2;

//D		angles.x = 2 * atan2(x,w);
//D		angles.z = 0;
	}
	else if (siny < -1.0f + FLT_TOLERANCE) // singularity at south pole
	{
		angles.y = (float) -FMath::Pi / 2;

//D		angles.x = -2 * atan2(x,w);
//D		angles.z = 0;
	}
	else
	{
		angles.y = asinf(siny);
		float cosy = cosf(angles.y);

		// I derived:
		angles.x = atan2f(2.0f * (x*w+y*z) / cosy, x*x+y*y - 1 - cosy); //		angles.x = atan2f(2*y*w-2*x*z, 1 - 2*y*y - 2*z*z);
//D		angles.z = atan2f(2*x*w-2*y*z, 1 - 2*x*x - 2*z*z);
	}

	return angles;
}

FMMatrix44 FMQuaternion::ToMatrix() const
{
	FMVector3 axis; float angle;
	ToAngleAxis(axis, angle);
	FMMatrix44 m;
	SetToMatrix(m);
	return FMMatrix44::AxisRotationMatrix(axis, angle);
}


void FMQuaternion::SetToMatrix(FMMatrix44& m) const
{
	//*(FMVector3*)m[0] = *this * FMVector3::XAxis;
	//*(FMVector3*)m[1] = *this * FMVector3::YAxis;
	//*(FMVector3*)m[2] = *this * FMVector3::ZAxis;

	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz, den;

	if (*this == FMQuaternion::Identity) 
	{
		m = FMMatrix44::Identity;
		return;		
	}
	// For unit q, just set s = 2.0; or or set xs = q.x + q.x, etc 
	den =  (x*x + y*y + z*z + w*w);
	if (den==0.0) {  s = (float)1.0; }
	else s = (float)2.0/den;

	xs = x * s;   ys = y * s;  zs = z * s;
	wx = w * xs;  wy = w * ys; wz = w * zs;
	xx = x * xs;  xy = x * ys; xz = x * zs;
	yy = y * ys;  yz = y * zs; zz = z * zs;

	m[0][0] = (float)1.0 - (yy +zz);
	m[1][0] = xy - wz; 
	m[2][0] = xz + wy; 

	m[0][1] = xy + wz; 
	m[1][1] = (float)1.0 - (xx +zz);
	m[2][1] = yz - wx; 

	m[0][2] = xz - wy; 
	m[1][2] = yz + wx; 
	m[2][2] = (float)1.0 - (xx + yy);
}


FMQuaternion FMQuaternion::EulerRotationQuaternion(float x, float y, float z)
{
	FMQuaternion qx(FMVector3::XAxis, x);
	FMQuaternion qy(FMVector3::YAxis, y);
	FMQuaternion qz(FMVector3::ZAxis, z);
	return qx * qy * qz;
}

static int next[3] = {1,2,0};
FMQuaternion FMQuaternion::MatrixRotationQuaternion(FMMatrix44& mat)
{
	FMQuaternion q;

	float tr,s;
	
	tr = 1.0f + mat[0][0] + mat[1][1] + mat[2][2];
	if (tr > 0.00001f)
	{
		s = (float)sqrt(tr) * 2.0f;
		q.x = (mat[1][2] - mat[2][1]) / s;
		q.y = (mat[2][0] - mat[0][2]) / s;
		q.z = (mat[0][1] - mat[1][0]) / s;
		q.w = s * 0.25f;
	}
	else if (mat[0][0] > mat[1][1])
	{
		s = sqrtf(1.0f + mat[0][0] - mat[1][1] - mat[2][2]) * 2.0f;
		q.x = 0.25f * s;
		q.y = (mat[0][1] + mat[1][0]) / s;
		q.z = (mat[2][0] + mat[0][2]) / s;
		q.w = (mat[1][2] - mat[2][1]) / s;
	}
	else if (mat[1][1] > mat[2][2])
	{
		s = sqrtf(1.0f + mat[1][1] - mat[0][0] - mat[2][2]) * 2.0f;
		q.x = (mat[0][1] + mat[1][0] ) / s;
		q.y = 0.25f * s;
		q.z = (mat[1][2] + mat[2][1] ) / s;
		q.w = (mat[2][0] - mat[0][2] ) / s;
	}
	else
	{
		s  = sqrtf(1.0f + mat[2][2] - mat[0][0] - mat[1][1]) * 2.0f;
		q.x = (mat[2][0] + mat[0][2] ) / s;
		q.y = (mat[1][2] + mat[2][1] ) / s;
		q.z = 0.25f * s;
		q.w = (mat[0][1] - mat[1][0] ) / s;
	}
	return q;
}

