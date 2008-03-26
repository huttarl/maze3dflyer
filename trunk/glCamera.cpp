// glCamera.cpp: implementation of the glCamera class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h> // temp for dbg
#include "glCamera.h"

extern void debugMsg(const char *str, ...);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

glCamera::glCamera()
{
	// Initalize all our member varibles.
	m_MaxPitchRate			= 0.0f;
	m_MaxHeadingRate		= 0.0f;
	m_HeadingDegrees		= 0.0f;
	m_PitchDegrees			= 0.0f;
	m_MaxAccel				= 0.0f;
	m_MaxVelocity			= 0.0f;
	m_ForwardVelocity		= 0.0f;
	m_SidewaysVelocity		= 0.0f;
}

glCamera::~glCamera()
{

}

extern GLvoid glPrint(const char *fmt, ...);
extern bool collide(glPoint &p, glVector &v);

// To do: maybe pass in collide() as a parameter.
// If so, also factor out movement from SetPerspective().

void glCamera::SetPerspective()
{
	GLfloat Matrix[16];
	glQuaternion q;
	glVector rightVector;
	unsigned char moving = (abs(m_SidewaysVelocity) + abs(m_ForwardVelocity) > 0.000001);

	// Make the Quaternions that will represent our rotations
	m_qPitch.CreateFromAxisAngle(1.0f, 0.0f, 0.0f, m_PitchDegrees);
	m_qHeading.CreateFromAxisAngle(0.0f, 1.0f, 0.0f, m_HeadingDegrees);
	// Combine the pitch and heading rotations and store the results in q
	q = m_qPitch * m_qHeading;
	q.CreateMatrix(Matrix);

	// Let OpenGL set our new prespective on the world!
	glMultMatrixf(Matrix);
	
	// Create a matrix from the pitch Quaternion and get the j vector 
	// for our direction.
	m_qPitch.CreateMatrix(Matrix);
	m_DirectionVector.j = Matrix[9];

	// Combine the heading and pitch rotations and make a matrix to get
	// the i and j vectors for our direction.
	q = m_qHeading * m_qPitch;
	q.CreateMatrix(Matrix);
	m_DirectionVector.i = Matrix[8];
	m_DirectionVector.k = Matrix[10];

	// Create a temp vector for sideways motion.
	// Since there is no "roll" in this camera model, sideways is
	// always horizontal.
	rightVector.j = 0;
	rightVector.i = m_DirectionVector.k;
	rightVector.k = -m_DirectionVector.i;

	// Scale the direction by our velocity.
	m_DirectionVector *= m_ForwardVelocity;
	rightVector *= m_SidewaysVelocity / rightVector.magnitude();
	m_DirectionVector += rightVector;

	// Check for collision and adjust position accordingly.
	// Increment our position by the vector if no collision.
	(void)collide(m_Position, m_DirectionVector);

	// Translate to our new position.
	glTranslatef(-m_Position.x, -m_Position.y, m_Position.z);

	glPrint("<%0.2f %0.2f %0.2f>", m_Position.x, m_Position.y, -m_Position.z);
}

void glCamera::ChangePitch(GLfloat degrees)
{
	if(fabs(degrees) < fabs(m_MaxPitchRate))
	{
		// Our pitch is less than the max pitch rate that we 
		// defined so lets increment it.
		m_PitchDegrees += degrees;
	}
	else
	{
		// Our pitch is greater than the max pitch rate that
		// we defined so we can only increment our pitch by the 
		// maximum allowed value.
		if(degrees < 0)
		{
			// We are pitching down so decrement
			m_PitchDegrees -= m_MaxPitchRate;
		}
		else
		{
			// We are pitching up so increment
			m_PitchDegrees += m_MaxPitchRate;
		}
	}

	// We don't want our pitch to run away from us. Although it
	// really doesn't matter I prefer to have my pitch degrees
	// within the range of -360.0f to 360.0f
	if(m_PitchDegrees > 360.0f)
	{
		m_PitchDegrees -= 360.0f;
	}
	else if(m_PitchDegrees < -360.0f)
	{
		m_PitchDegrees += 360.0f;
	}
}

void glCamera::ChangeHeading(GLfloat degrees)
{
	if(fabs(degrees) < fabs(m_MaxHeadingRate))
	{
		// Our Heading is less than the max heading rate that we 
		// defined so lets increment it but first we must check
		// to see if we are inverted so that our heading will not
		// become inverted.
		if(m_PitchDegrees > 90 && m_PitchDegrees < 270 || (m_PitchDegrees < -90 && m_PitchDegrees > -270))
		{
			m_HeadingDegrees -= degrees;
		}
		else
		{
			m_HeadingDegrees += degrees;
		}
	}
	else
	{
		// Our heading is greater than the max heading rate that
		// we defined so we can only increment our heading by the 
		// maximum allowed value.
		if(degrees < 0)
		{
			// Check to see if we are upside down.
			if((m_PitchDegrees > 90 && m_PitchDegrees < 270) || (m_PitchDegrees < -90 && m_PitchDegrees > -270))
			{
				// Ok we would normally decrement here but since we are upside
				// down then we need to increment our heading
				m_HeadingDegrees += m_MaxHeadingRate;
			}
			else
			{
				// We are not upside down so decrement as usual
				m_HeadingDegrees -= m_MaxHeadingRate;
			}
		}
		else
		{
			// Check to see if we are upside down.
			if(m_PitchDegrees > 90 && m_PitchDegrees < 270 || (m_PitchDegrees < -90 && m_PitchDegrees > -270))
			{
				// Ok we would normally increment here but since we are upside
				// down then we need to decrement our heading.
				m_HeadingDegrees -= m_MaxHeadingRate;
			}
			else
			{
				// We are not upside down so increment as usual.
				m_HeadingDegrees += m_MaxHeadingRate;
			}
		}
	}
	
	// We don't want our heading to run away from us either. Although it
	// really doesn't matter I prefer to have my heading degrees
	// within the range of -360.0f to 360.0f
	if(m_HeadingDegrees > 360.0f)
	{
		m_HeadingDegrees -= 360.0f;
	}
	else if(m_HeadingDegrees < -360.0f)
	{
		m_HeadingDegrees += 360.0f;
	}
}

// To do: if m_MaxVelocity is really max overall velocity, we need to take into account sideways velocity.
void glCamera::AccelForward(GLfloat accel)
{
	//debugMsg("%2.2f + AccelForward(%2.2f): ", m_ForwardVelocity, accel);
	GLfloat nv, mfv2; // temp new velocity values
	if (accel > m_MaxAccel) accel = m_MaxAccel;
	else if (accel < -m_MaxAccel) accel = -m_MaxAccel;

	nv = m_ForwardVelocity + accel;
	mfv2 = m_MaxVelocity * m_MaxVelocity - m_SidewaysVelocity * m_SidewaysVelocity; // max forward velocity squared
	if (nv * nv > mfv2) // too high
		nv = (nv > 0 ? sqrt(mfv2) : -sqrt(mfv2));

	m_ForwardVelocity = nv;
	//debugMsg("= %2.2f\n", m_ForwardVelocity);
}

void glCamera::AccelSideways(GLfloat accel)
{
	GLfloat nv, msv2; // temp new velocity values
	if (accel > m_MaxAccel) accel = m_MaxAccel;
	else if (accel < -m_MaxAccel) accel = -m_MaxAccel;

	nv = m_SidewaysVelocity + accel;
	msv2 = m_MaxVelocity * m_MaxVelocity - m_ForwardVelocity * m_ForwardVelocity; // max Sideways velocity squared
	if (nv * nv > msv2) // too high
		nv = (nv > 0 ? sqrt(msv2) : -sqrt(msv2));

	m_SidewaysVelocity = nv;
}


void glCamera::MoveForward(GLfloat distance)
{
	m_ForwardVelocity = 0.0f;
	AccelForward(distance);
}

void glCamera::MoveSideways(GLfloat distance)
{
	m_SidewaysVelocity = 0.0f;
	AccelSideways(distance);
}
