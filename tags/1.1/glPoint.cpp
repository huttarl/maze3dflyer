// glPoint.cpp: implementation of the glPoint class.
//
//////////////////////////////////////////////////////////////////////

#include "glPoint.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

glPoint::glPoint()
{
	x = y = z = 0.0f;
}

glPoint::~glPoint()
{

}

void glPoint::operator +=(glVector v)
{
	x += v.i;
	y += v.j;
	z += v.k;
}

void glPoint::operator -=(glVector v)
{
	x -= v.i;
	y -= v.j;
	z -= v.k;
}
