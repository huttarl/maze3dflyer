#ifndef __Picture_h__
#define __Picture_h__

#include "CellCoord.h"
#include "Wall.h"
#include "Image.h"

// An instance of an image hung on a wall somewhere.
class Picture {
public:
   CellCoord where;
   Wall *wall;
   Image *image;
   char dir;
   Quad quad; // vertices of where to draw picture
   Quad frameQuad; // vertices of where to draw frame

   void draw(void);
   void setupVertices(void);

   Picture() {
      wall = (Wall *)0;
      image = (Image *)0;
      dir = '\0';
   }
};

#endif // __Picture_h__
