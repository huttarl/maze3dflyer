// glCamera.h: interface for the glCamera class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLCAMERA_H__8E3CD02E_6D82_437E_80DA_50023C60C146__INCLUDED_)
#define AFX_GLCAMERA_H__8E3CD02E_6D82_437E_80DA_50023C60C146__INCLUDED_

#include <windows.h>		// Header File For Windows
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library

#include "glQuaternion.h"
#include "glPoint.h"
#include "glVector.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class glCamera  
{
public:
	GLfloat m_MaxPitchRate;
	GLfloat m_MaxPitch;
	GLfloat m_MaxHeadingRate;
	GLfloat m_HeadingDegrees;
	GLfloat m_PitchDegrees;
	GLfloat m_MaxVelocity;
	GLfloat m_MaxAccel;
	GLfloat m_ForwardVelocity;
	GLfloat m_SidewaysVelocity;
	glQuaternion m_qHeading;
	glQuaternion m_qPitch;
	glPoint m_Position;
	glVector m_DirectionVector;
        GLfloat m_framerate;
        GLfloat m_targetframerate;      // target framerate: acceleration / velocity is geared toward this.
        GLfloat m_minframerate; // assume this framerate even if we're not achieving it.
        GLfloat m_framerateAdjust; // ratio of target framerate to actual framerate: for adjusting framerate-dependent speed/accel.

	void AccelForward(GLfloat vel);
	void AccelSideways(GLfloat vel);
	void MoveForward(GLfloat distance);
	void MoveSideways(GLfloat distance);
	void ChangeHeading(GLfloat degrees);
	void ChangePitch(GLfloat degrees);
	void SetPerspective(void);
	void glCamera::SnapToGrid(void);
	void glCamera::GoTo(GLfloat nx, GLfloat ny, GLfloat nz, GLfloat npitch, GLfloat nheading);
	glCamera();
	virtual ~glCamera();

};

#endif // !defined(AFX_GLCAMERA_H__8E3CD02E_6D82_437E_80DA_50023C60C146__INCLUDED_)
