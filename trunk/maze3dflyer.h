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

extern const int numFilters;		// How many filters for each texture
extern GLuint filter;				// Which filter to use
extern GLuint textures[]; // texture indexes ("names")
extern GLuint facadeDL; // needed by Wall.cpp

typedef enum { ground, wall1, wall2, portal, roof, sky } Material;

extern GLUquadricObj *quadric;	

#endif                  /* __maze3dflyer_h__ */
