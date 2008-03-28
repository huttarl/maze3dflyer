#ifndef __maze3dflyer_h__
#define __maze3dflyer_h__


#include <stdlib.h>
#include <gl/glut.h>
#include "glCamera.h"

extern void debugMsg(const char *str, ...);
extern void errorMsg(const char *str, ...);

extern glCamera Cam;				
extern float	keyTurnRate; // how fast to turn in response to keys
extern float	keyAccelRate; // how fast to accelerate in response to keys
extern float	keyMoveRate;  // how fast to move in response to keys

extern GLUquadricObj *quadric;	

#endif                  /* __maze3dflyer_h__ */
