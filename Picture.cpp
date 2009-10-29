#include "Picture.h"
#include "Maze3D.h"
#include "maze3dflyer.h"

// Draw a quad for picture. Do not use within glBegin/End (e.g. of QUADS)
void Picture::draw(void) {
   Vertex *qv = quad.vertices;

   //##TODO: would like to have a "frame" behind the pic.

   // picture texture
   glBindTexture(GL_TEXTURE_2D, textureId);
   glColor3f(1.0f, 1.0f, 1.0f);
   // Process a quad
   glBegin(GL_QUADS);

   switch (dir) {
      case 'x': glNormal3f(wall->outsidePositive ? 1.0f : -1.0f, 0.0f, 0.0f); break;
      case 'y': glNormal3f(0.0f, wall->outsidePositive ? 1.0f : -1.0f, 0.0f); break;
      case 'z': glNormal3f(0.0f, 0.0f, wall->outsidePositive ? 1.0f : -1.0f); break;
      default: errorMsg("Invalid dir in Picture::draw()\n");
   }

   for (int i=0; i < 4; i++) {
      //if (firstTime) debugMsg("(%1.1f %1.1f %1.1f) ", qv[i].x, qv[i].y, qv[i].z);
      glTexCoord2f(qv[i].u, qv[i].v); glVertex3f(qv[i].x, qv[i].y, qv[i].z);
   }

   glEnd();
}

void Picture::setupVertices(void) {
   GLfloat ep = 0.0001; // epsilon

   //##TODO generalize to all dirs

   // Copy all vertex coords for starters; then change some.
   quad = wall->quad;

   Vertex *pv, *wv;

   for (int i = 0; i < 4; i++) {
      pv = &(quad.vertices[i]);
      // wv = &(wall->quad.vertices[i]);
      pv->x -= ep;
   }
   // set bottom and top of picture
   quad.vertices[0].y = quad.vertices[1].y = (where.y + 0.3) * Maze3D::cellSize;
   quad.vertices[2].y = quad.vertices[3].y = (where.y + 0.9) * Maze3D::cellSize;
   // sides
   quad.vertices[0].z = quad.vertices[3].z = (where.z + 0.25) * Maze3D::cellSize;
   quad.vertices[1].z = quad.vertices[2].z = (where.z + 0.75) * Maze3D::cellSize;
}
