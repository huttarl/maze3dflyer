#include "Wall.h"
#include "Maze3D.h"
#include "maze3dflyer.h"

// interpolate linearly from a to b by p (0 to 1)
inline double interp(double a, double b, double p) { return a + (b - a) * p; }

// Within a glBegin/glEnd, draw a quad for this wall.
void Wall::draw(char dir) {
	switch (dir) {
		case 'x': glNormal3f(outsidePositive ? 1.0f : -1.0f, 0.0f, 0.0f); break;
		case 'y': glNormal3f(0.0f, outsidePositive ? 1.0f : -1.0f, 0.0f); break;
		case 'z': glNormal3f(0.0f, 0.0f, outsidePositive ? 1.0f : -1.0f); break;
		default: errorMsg("Invalid dir in Wall::draw('%c')\n", dir);
	}
	Vertex *qv = quad.vertices;
        // ### TODO: this is not a very efficient way to draw bars.
        // Could use display lists if it's an issue.
        // Maybe use a completely different texture... though watch out for messing up the texture of
        // other walls in the loop calling draw().
        if (seeThrough) {
           const int numBars = 5;
           double barSize = 0.06f;
           double gapSize = (1.0 - barSize * numBars) / (numBars - 1);
           for (int i=0; i < numBars; i++) {
              double barStart = i * (barSize + gapSize);
              double barEnd = barStart + barSize;
              glTexCoord2f(interp(qv[0].u, qv[1].u, barStart), interp(qv[0].v, qv[1].v, barStart));
              glVertex3f(interp(qv[0].x, qv[1].x, barStart), interp(qv[0].y, qv[1].y, barStart), interp(qv[0].z, qv[1].z, barStart));

              glTexCoord2f(interp(qv[0].u, qv[1].u, barEnd), interp(qv[0].v, qv[1].v, barEnd));
              glVertex3f(interp(qv[0].x, qv[1].x, barEnd), interp(qv[0].y, qv[1].y, barEnd), interp(qv[0].z, qv[1].z, barEnd));

              glTexCoord2f(interp(qv[3].u, qv[2].u, barEnd), interp(qv[3].v, qv[2].v, barEnd));
              glVertex3f(interp(qv[3].x, qv[2].x, barEnd), interp(qv[3].y, qv[2].y, barEnd), interp(qv[3].z, qv[2].z, barEnd));

              glTexCoord2f(interp(qv[3].u, qv[2].u, barStart), interp(qv[3].v, qv[2].v, barStart));
              glVertex3f(interp(qv[3].x, qv[2].x, barStart), interp(qv[3].y, qv[2].y, barStart), interp(qv[3].z, qv[2].z, barStart));

              glTexCoord2f(interp(qv[1].u, qv[2].u, barStart), interp(qv[1].v, qv[2].v, barStart));
              glVertex3f(interp(qv[1].x, qv[2].x, barStart), interp(qv[1].y, qv[2].y, barStart), interp(qv[1].z, qv[2].z, barStart));

              glTexCoord2f(interp(qv[1].u, qv[2].u, barEnd), interp(qv[1].v, qv[2].v, barEnd));
              glVertex3f(interp(qv[1].x, qv[2].x, barEnd), interp(qv[1].y, qv[2].y, barEnd), interp(qv[1].z, qv[2].z, barEnd));

              glTexCoord2f(interp(qv[0].u, qv[3].u, barEnd), interp(qv[0].v, qv[3].v, barEnd));
              glVertex3f(interp(qv[0].x, qv[3].x, barEnd), interp(qv[0].y, qv[3].y, barEnd), interp(qv[0].z, qv[3].z, barEnd));

              glTexCoord2f(interp(qv[0].u, qv[3].u, barStart), interp(qv[0].v, qv[3].v, barStart));
              glVertex3f(interp(qv[0].x, qv[3].x, barStart), interp(qv[0].y, qv[3].y, barStart), interp(qv[0].z, qv[3].z, barStart));
              // ### TODO: these grills don't contrast well with the same texture on a similar wall behind them.
              // Should draw black line squares around the holes. Do this the same way as the 'grid' option.
           }
        } else {
	     for (int i=0; i < 4; i++) {
		  //if (firstTime) debugMsg("(%1.1f %1.1f %1.1f) ", qv[i].x, qv[i].y, qv[i].z);
		  glTexCoord2f(qv[i].u, qv[i].v); glVertex3f(qv[i].x, qv[i].y, qv[i].z);
	     }
        }

	//if (firstTime) debugMsg("\n");
}

// Outside a glBegin/glEnd, draw a visual marker for the exit (or entrance) at this wall.
// CellCoord x, y, z helps us know which way is in vs. out.
void Wall::drawExit(int x, int y, int z, bool isEntrance) {
   Vertex *qv = quad.vertices;
   // center for rotating disc:
   GLfloat cx = qv[0].x + Maze3D::cellSize * 0.5f,
	    cy = qv[0].y + Maze3D::cellSize * 0.5f,
	    cz = qv[0].z + Maze3D::cellSize * 0.5f; // center of disc
   //if (firstTime)
   //	debugMsg("drawExit(%d, %d, %d, %s): %f, %f, %f\n",
   //		x, y, z, isEntrance ? "entrance" : "exit",
   //		qv[0].x, qv[0].y, qv[0].z);

   glPushMatrix();

   // rotate disc from z plane to x or y plane if nec.
   if (qv[0].x == qv[2].x) { // exit wall is in X plane
	    cx = qv[0].x;
	    glTranslatef(cx, cy, cz);
	    glRotatef(90.0f, 0.0f, 1.0f, 0.0f); // rotate disc around y axis
   } else if (qv[0].y == qv[2].y) { // exit wall is in Y plane
	    cy = qv[0].y;
	    glTranslatef(cx, cy, cz);
	    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // rotate disc around x axis
   } else {
	    cz = qv[0].z;
	    glTranslatef(cx, cy, cz);
   }

   // draw facade: first reset color, then call display list.
   glColor3f(1.0f, 1.0f, 1.0f);
   glCallList(facadeDL);

   // no texture
   glBindTexture(GL_TEXTURE_2D, GL_NONE);
   // color: red or green
   if (isEntrance) glColor3f(0.4f, 1.0f, 0.4f);
   else glColor3f(1.0f, 0.4f, 0.4f);


   // spin polygon (disc)
   maze.exitRot += 1.25f * Cam.m_framerateAdjust;
   glRotatef(isEntrance ? maze.exitRot : -maze.exitRot, 0.0f, 0.0f, 1.0f);

   // gluDisk(quadric, innerRadius, outerRadius, slices, loops)
   gluDisk(diskQuadric, Maze3D::exitHoleRadius * 0.8, Maze3D::exitHoleRadius, 13, 3);

   // If there are prizes left, exit should be closed.
   if (!isEntrance && maze.isExitLocked()) {
      glRectf(-Maze3D::exitHoleRadius * 0.1, -Maze3D::exitHoleRadius * 0.9,
         Maze3D::exitHoleRadius * 0.1, Maze3D::exitHoleRadius * 0.9);
   }

   glPopMatrix();
}
