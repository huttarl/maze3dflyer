#include "Picture.h"
#include "Maze3D.h"
#include "maze3dflyer.h"

// Draw a quad for picture. Do not use within glBegin/End (e.g. of QUADS)
void Picture::draw(void) {

   //##TODO: would like to have a "frame" behind the pic.

   switch (dir) {
      case 'x': glNormal3f(wall->outsidePositive ? 1.0f : -1.0f, 0.0f, 0.0f); break;
      case 'y': glNormal3f(0.0f, wall->outsidePositive ? 1.0f : -1.0f, 0.0f); break;
      case 'z': glNormal3f(0.0f, 0.0f, wall->outsidePositive ? 1.0f : -1.0f); break;
      default: errorMsg("Invalid dir in Picture::draw()\n");
   }

   Vertex *qv = frameQuad.vertices;

   glColor3f(0.0f, 0.0f, 0.0f); // black
   // no texture
   glBindTexture(GL_TEXTURE_2D, GL_NONE);
   glBegin(GL_QUADS);
   for (int i=0; i < 4; i++) {
      glTexCoord2f(qv[i].u, qv[i].v); glVertex3f(qv[i].x, qv[i].y, qv[i].z);
   }
   glEnd();

   qv = quad.vertices;
   glColor3f(1.0f, 1.0f, 1.0f); // white
   // picture texture
   glBindTexture(GL_TEXTURE_2D, image->textureId);
   glBegin(GL_QUADS);
   for (int i=0; i < 4; i++) {
      glTexCoord2f(qv[i].u, qv[i].v); glVertex3f(qv[i].x, qv[i].y, qv[i].z);
   }
   glEnd();
}

void Picture::setupVertices(void) {
   GLfloat ep = 0.001; // epsilon

   //##TODO generalize to all directions

   Vertex *qv = quad.vertices;

   //##TODO: refactor this stuff out into a function call

   // Copy all vertex coords for starters; then change some.
   quad = wall->quad; // This is not just a copy-by-reference!

   for (int i = 0; i < 4; i++) {
      // wv = &(wall->quad.vertices[i]);
      qv[i].x -= ep;
   }

   float top = 0.9f, bottom = 0.3f, side = 0.25f;
   // set bottom and top of picture
   qv[0].y = qv[1].y = (where.y + bottom) * Maze3D::cellSize;
   qv[2].y = qv[3].y = (where.y + top) * Maze3D::cellSize;
   // sides
   qv[0].z = qv[3].z = (where.z + side) * Maze3D::cellSize;
   qv[1].z = qv[2].z = (where.z + 1.0 - side) * Maze3D::cellSize;

   frameQuad = wall->quad; // deep copy

   qv = frameQuad.vertices;

   for (int i = 0; i < 4; i++) {
      // wv = &(wall->quad.vertices[i]);
      qv[i].x -= ep / 2.0;
   }

   top = 0.925f, bottom = 0.275f, side = 0.225f;
   // set bottom and top of picture
   qv[0].y = qv[1].y = (where.y + bottom) * Maze3D::cellSize;
   qv[2].y = qv[3].y = (where.y + top) * Maze3D::cellSize;
   // sides
   qv[0].z = qv[3].z = (where.z + side) * Maze3D::cellSize;
   qv[1].z = qv[2].z = (where.z + 1.0 - side) * Maze3D::cellSize;

   //##TODO: rotate quads around center to desired direction
}
