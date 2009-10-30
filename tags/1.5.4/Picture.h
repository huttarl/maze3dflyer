#ifndef __Picture_h__
#define __Picture_h__

// #include <gl/glut.h> // causes error in redefinition of exit()
typedef unsigned int GLuint;

#include "CellCoord.h"
#include "Wall.h"

// An instance of an image hung on a wall somewhere.
class Picture {
public:
   CellCoord where;
   Wall *wall;
   GLuint textureId;
   char dir;
   Quad quad; // vertices of where to draw

   void draw(void);
   void setupVertices(void);

   Picture() {
      wall = (Wall *)0;
      textureId = -1;
      dir = '\0';
   }
};

#endif // __Picture_h__
