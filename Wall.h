#ifndef __Wall_h__
#define __Wall_h__

#include <stdlib.h>
#include <gl/glut.h>

//#include "maze3dflyer.h"

typedef struct tagVERTEX
{
	float x, y, z;
	float u, v;
} Vertex;

typedef struct tagQUAD
{
	Vertex vertices[4];
} Quad;

class Picture;

class Wall {
public:
   enum WallState { UNINITIALIZED, OPEN, CLOSED } state;
   // outsidePositive: true if "outside" of face is in positive axis direction.
   // only applies to CLOSED walls.
   bool outsidePositive;
   // seeThrough: true if you can see through this wall (only applies to CLOSED walls)
   bool seeThrough;
   Quad quad;
   Wall() { state = UNINITIALIZED; outsidePositive = false; seeThrough = false; }

   void Wall::draw(char dir);
   void Wall::drawExit(int x, int y, int z, bool isEntrance);
};

#endif /* __Wall_h__ */
