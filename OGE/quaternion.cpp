#include "quaternion.h"

#include "SDL_OpenGL.h"

#include <math.h>

#define PI (3.14159265f)

quaternion::quaternion()
{
	m_x = m_y = m_z = 0.0f;
	m_w = 1.0f;
}

quaternion::~quaternion()
{

}

void quaternion::CreateFromAngles(real p_roll, real p_pitch, real p_yaw)
{
	float cr, cp, cy, sr, sp, sy, cpcy, spsy;


	// calculate trig identities
	cr = cos(p_roll/2);

	cp = cos(p_pitch/2);
	cy = cos(p_yaw/2);


	sr = sin(p_roll/2);
	sp = sin(p_pitch/2);
	sy = sin(p_yaw/2);
	
	cpcy = cp * cy;
	spsy = sp * sy;


	m_w = cr * cpcy + sr * spsy;
	m_x = sr * cpcy - cr * spsy;
	m_y = cr * sp * cy + sr * cp * sy;
	m_z = cr * cp * sy - sr * sp * cy;
}

void quaternion::CreateFromAxisAngle(real x, real y, real z, real degrees)
{
	// First we want to convert the degrees to radians 
	// since the angle is assumed to be in radians
	real angle = (real)((degrees / 180.0f) * PI);

	// Here we calculate the sin( theta / 2) once for optimization
	real result = (real)sin( angle / 2.0f );
		
	// Calcualte the w value by cos( theta / 2 )
	m_w = (real)cos( angle / 2.0f );

	// Calculate the x, y and z of the quaternion
	m_x = (real)(x * result);
	m_y = (real)(y * result);
	m_z = (real)(z * result);
}

void quaternion::CreateMatrix(GLfloat *pMatrix) const
{
	// Make sure the matrix has allocated memory to store the rotation data
	if(!pMatrix) {
		return;
	}

	// This is in column major
	
	// First row
	pMatrix[0] = 1.0f - 2.0f * ( m_y * m_y + m_z * m_z ); 
	pMatrix[1] = 2.0f * (m_x * m_y + m_z * m_w);
	pMatrix[2] = 2.0f * (m_x * m_z - m_y * m_w);
	pMatrix[3] = 0.0f;  

	// Second row
	pMatrix[4] = 2.0f * ( m_x * m_y - m_z * m_w );  
	pMatrix[5] = 1.0f - 2.0f * ( m_x * m_x + m_z * m_z ); 
	pMatrix[6] = 2.0f * (m_z * m_y + m_x * m_w );  
	pMatrix[7] = 0.0f;  

	// Third row
	pMatrix[8] = 2.0f * ( m_x * m_z + m_y * m_w );
	pMatrix[9] = 2.0f * ( m_y * m_z - m_x * m_w );
	pMatrix[10] = 1.0f - 2.0f * ( m_x * m_x + m_y * m_y );  
	pMatrix[11] = 0.0f;  

	// Fourth row
	pMatrix[12] = 0;  
	pMatrix[13] = 0;  
	pMatrix[14] = 0;  
	pMatrix[15] = 1.0f;

	// Now pMatrix[] is a 4x4 homogeneous matrix that can be applied to an OpenGL Matrix
}

quaternion quaternion::operator *(quaternion q) const
{
	quaternion r;

	r.m_w = m_w*q.m_w - m_x*q.m_x - m_y*q.m_y - m_z*q.m_z;
	r.m_x = m_w*q.m_x + m_x*q.m_w + m_y*q.m_z - m_z*q.m_y;
	r.m_y = m_w*q.m_y + m_y*q.m_w + m_z*q.m_x - m_x*q.m_z;
	r.m_z = m_w*q.m_z + m_z*q.m_w + m_x*q.m_y - m_y*q.m_x;
	
	return(r);
}
