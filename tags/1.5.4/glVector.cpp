// glVector.cpp: implementation of the glVector class.
//
//////////////////////////////////////////////////////////////////////

#include <math.h>
#include "glVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

glVector::glVector()
{
	i = j = k = 0.0f;
}

glVector::~glVector()
{

}

void glVector::operator *=(GLfloat scalar)
{
	i *= scalar;
	j *= scalar;
	k *= scalar;
}

void glVector::operator +=(glVector v)
{
	i += v.i;
	j += v.j;
	k += v.k;
}

GLfloat glVector::magnitude()
{
	return sqrt(i*i + j*j + k*k);
}

