#include "Maze3D.h"
#include "maze3dflyer.h"

// Two different ways to draw outline. Define exactly one.
#define OutlineWithLines 1
// I don't recommend outlining with cylinders. At close range, they look like crude cylinders
// (square prisms). At far range they look like dotted lines.
// #define OutlineWithCylinders 1

// Two different ways to draw route. Define exactly one.
// #define RouteWithLines 1
#define RouteWithCylinders 1

// size of one cell in world coordinates. Some code may depend on cellSize = 1.0
const float Maze3D::cellSize = 1.0f;
// margin around walls that we can't collide with; should be << cellSize.
const float Maze3D::wallMargin = 0.2f;
// apparently these statics have to be defined outside of the class def.
float Maze3D::exitRot = 0.0f;
float Maze3D::exitHoleRadius = 0.3f;
float Maze3D::exitThickness = Maze3D::cellSize * 0.06;
bool Maze3D::checkCollisions = true;
clock_t Maze3D::whenEntered = (clock_t)0, Maze3D::whenSolved = (clock_t)0, Maze3D::lastSolvedTime = (clock_t)0;
bool Maze3D::hasFoundExit = false, Maze3D::newBest = false;
float Maze3D::edgeRadius = 0.006 * cellSize;
float Maze3D::routeRadius = 0.01 * cellSize;
int Maze3D::seeThroughRarity = 6;

Maze3D::Maze3D(int _w, int _h, int _d, int _s, int _b) {
   w = _w, h = _h, d = _d;
   sparsity = _s;
   branchClustering = _b;
   //cellSize = 1.0f;
   //wallMargin = 0.11f;
   //checkCollisions = true;
   //exitRot = 0.0f;
   cells = NULL;
   xWalls = NULL;
   yWalls = NULL;
   zWalls = NULL;
   solutionRoute = NULL;
   nPrizes = 0; nPrizesLeft = 0;
   nPictures = 0;
   hitLockedExit = isGenerating = false;
}

// setDims should be called only before SetupWorld().
void Maze3D::setDims(int _w, int _h, int _d, int _s, int _b) {
   w = _w, h = _h, d = _d;
   sparsity = _s;
   branchClustering = _b;
}

void Maze3D::randomizeDims(void) {
   maze.w = (rand() % 5) + (rand() % 5) + 2;
   maze.h = (rand() % 5) + (rand() % 5) + 2;
   maze.d = (rand() % 5) + (rand() % 5) + 2;
   maze.sparsity = rand() % 2 + 2;
   debugMsg("Random maze size: %dx%dx%d/%d\n",
      maze.w, maze.h, maze.d, maze.sparsity);
}

// slightly increase the dimensions of the maze, to correspond to
// given difficulty level.
void Maze3D::incrementDims(int level) {
   // int v = volume();
   // Randomly pick an axis to increase, with the smallest axis being most likely.
   int whichAxis = rand() % (w*2 + h*2 + d*2);
   if (whichAxis < w + h && d < dMax) d++;
   else if (whichAxis < w + h + w + d && h < hMax) h++;
   else if (w < wMax) w++;
   else {
      debugMsg("Maze dims maxed out.\n");
      return;
   }

   int minDim = min(w, min(h, d));
   if (minDim < 3) sparsity = 2;
   else {
      // This is my pseudoscientific attempt at approximating a normal distribution.
      // r is a number in [0,1], normal distribution
      float r = ((rand() % 98) + (rand() % 99) + 0.0) / 195.0;
      // Put the bump at the low end, not in the middle.
      r = abs(r - 0.5) * 2.0;
      // bias against the highest number (minDim) by subtracting an extra 0.2.
      // round the result by adding 0.5 and casting to int.
      sparsity = int(2.0 + (minDim - 2.0) * r + 0.5);
   }
}

bool Maze3D::IsInside(glPoint p) {
   CellCoord cc; //TODO: use a static one
   //TODO: should this function be a member of CellCoord?
   p.z = -p.z;
   cc.init(p);
   return (cc.isCellPassageSafe());
   //    return (maze.cells[x][y][z].state == Cell::passage);
}

extern bool firstTime; // for debugging

// Draw an outline edge that is parallel to the X axis, if it ought to be drawn.
// Assumes i < w.
// Outputs two vertices for inclusion in a glBegin/End(GL_LINES).
// If forbidden is true, draw only edges of "forbidden" cells.
// Otherwise, draw only edges of "passage" cells.
void Maze3D::drawXEdge(int i, int j, int k, bool forbidden) {
   bool goAhead;
   if (forbidden) {
      goAhead =
         (j < w && ((k < d && cells[i][j][k].state == Cell::forbidden)
                 || (k > 0 && cells[i][j][k-1].state == Cell::forbidden)))
         ||
         (j > 0 && ((k < d && cells[i][j-1][k].state == Cell::forbidden)
                 || (k > 0 && cells[i][j-1][k-1].state == Cell::forbidden)));
   } else {
      bool jplus, jminus, kplus, kminus;
      // are xy walls in +y/-y directions closed?
      jplus  = j < h && IsWall(&zWalls[i][j  ][k]);
      jminus = j > 0 && IsWall(&zWalls[i][j-1][k]);;
      // are xz walls in +z/-z directions closed?
      kplus  = k < d && IsWall(&yWalls[i][j][k  ]);
      kminus = k > 0 && IsWall(&yWalls[i][j][k-1]);

      // draw an edge only if there are two walls at right angles to e.o.
      goAhead = (jplus || jminus) && (kplus || kminus);
   }

   // For now, just draw outlines for external edges
   //TODO: consider internal edges too

   if (goAhead) {
      // For now, we'll try lines, not quads; default width (prefer 1 pixel)
      // glColor3f(1.0, 0.0, 0.0); // red - debugging
      glvc(i, j, k);
      glvc((i + 1), j, k);
   }
}

// Draw an outline edge that is parallel to the Y axis, if it ought to be drawn.
// Assumes j < h.
// Outputs two vertices for inclusion in a glBegin/End(GL_LINES).
// If forbidden is true, draw only edges of "forbidden" cells.
// Otherwise, draw only edges of "passage" cells.
void Maze3D::drawYEdge(int i, int j, int k, bool forbidden) {
   bool goAhead;
   if (forbidden) {
      goAhead =
         (i < w && ((k < d && cells[i][j][k].state == Cell::forbidden)
                 || (k > 0 && cells[i][j][k-1].state == Cell::forbidden)))
         ||
         (i > 0 && ((k < d && cells[i-1][j][k].state == Cell::forbidden)
                 || (k > 0 && cells[i-1][j][k-1].state == Cell::forbidden)));
   } else {
      bool iplus, iminus, kplus, kminus;
      // are xy walls in +x/-x directions closed?
      iplus  = i < w && IsWall(&zWalls[i  ][j][k]);
      iminus = i > 0 && IsWall(&zWalls[i-1][j][k]);
      // are yz walls in +z/-z directions closed?
      kplus  = k < d && IsWall(&xWalls[i][j][k  ]);
      kminus = k > 0 && IsWall(&xWalls[i][j][k-1]);

      goAhead = (iplus || iminus) && (kplus || kminus);
   }

   // draw an edge only if there are two walls at right angles to e.o.
   if (goAhead) {
      // glColor3f(0.0, 1.0, 0.0); // green - debugging
      glvc(i, j, k);
      glvc(i, (j + 1), k);
   }
}

// Draw an outline edge that is parallel to the Z axis, if it ought to be drawn.
// Assumes k < d.
// Outputs two vertices for inclusion in a glBegin/End(GL_LINES).
// If forbidden is true, draw only edges of "forbidden" cells.
// Otherwise, draw only edges of "passage" cells.
void Maze3D::drawZEdge(int i, int j, int k, bool forbidden) {
   bool goAhead;
   if (forbidden) {
      goAhead =
         (i < w && ((j < d && cells[i][j  ][k].state == Cell::forbidden)
                 || (j > 0 && cells[i][j-1][k].state == Cell::forbidden)))
         ||
         (i > 0 && ((j < d && cells[i-1][j  ][k].state == Cell::forbidden)
                 || (j > 0 && cells[i-1][j-1][k].state == Cell::forbidden)));
   } else {
      bool iplus, iminus, jplus, jminus;
      // are xz walls in +x/-x directions closed?
      iplus  = i < w && IsWall(&yWalls[i  ][j][k]);
      iminus = i > 0 && IsWall(&yWalls[i-1][j][k]);
      // are yz walls in +y/-y directions closed?
      jplus  = j < h && IsWall(&xWalls[i][j  ][k]);
      jminus = j > 0 && IsWall(&xWalls[i][j-1][k]);

      // draw an edge only if there are two walls at right angles to e.o.
      goAhead = (iplus || iminus) && (jplus || jminus);
   }

   // draw an edge only if there are two walls at right angles to e.o.
   if (goAhead) {
      // glColor3f(0.0, 0.0, 1.0); // blue - debugging
#ifdef OutlineWithLines
      glvc(i, j, k);
      glvc(i, j, (k + 1));
#else
      glPushMatrix();
      glTranslatef(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
      // quadric, base, top, height, slices, stacks
      gluCylinder(cylQuadric, edgeRadius, edgeRadius, maze.cellSize, 4, 1);
      glPopMatrix();
#endif // OutlineWithLines

   }
}


// draw dark edges along outlines
void Maze3D::drawOutline(void) {
   // for smooth lines, need to turn on blending.
   // see http://www.opengl.org/resources/faq/technical/rasterization.htm#rast0150
   glColor3f(0.3, 0.3, 0.3);
   glLineWidth(1.0); // for outline. 1 is default anyway.

#ifdef OutlineWithLines
   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBindTexture(GL_TEXTURE_2D, GL_NONE); // no texture
   glBegin(GL_LINES); // using lines for now
#endif //OutlineWithLines
   for (int i=0; i <= w; i++)
      for (int j=0; j <= h; j++)
         for (int k=0; k <= d; k++) {
            // For every vertex in the maze, draw up to 3 edges
            if (i < w) drawXEdge(i, j, k);
            if (j < h) drawYEdge(i, j, k);
            if (k < d) drawZEdge(i, j, k);
         }
#ifdef OutlineWithLines
   glEnd(); // GL_LINES
   glPopAttrib();   
#endif //OutlineWithLines
}

// draw outlines of "forbidden" cells during maze generation
void Maze3D::drawForbidden(void) {
   // for smooth lines, need to turn on blending.
   // see http://www.opengl.org/resources/faq/technical/rasterization.htm#rast0150
   glColor3f(1.0, 0.3, 0.3);  // red
   glLineWidth(2.0); // 1 is default anyway.

   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBindTexture(GL_TEXTURE_2D, GL_NONE); // no texture
   glBegin(GL_LINES); // using lines for now
   for (int i=0; i <= w; i++)
      for (int j=0; j <= h; j++)
         for (int k=0; k <= d; k++) {
            // For every vertex in the maze, draw up to 3 edges
            if (i < w) drawXEdge(i, j, k, true);
            if (j < h) drawYEdge(i, j, k, true);
            if (k < d) drawZEdge(i, j, k, true);
         }
   glEnd(); // GL_LINES
   glPopAttrib();   
}

// Draw the cubes in the generating queue.
void Maze3D::drawQueue(void) {
   glColor3f(0.3, 0.3, 1.0); // blue
   glLineWidth(3.0);
   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glBindTexture(GL_TEXTURE_2D, GL_NONE); // no texture
   glBegin(GL_LINES);

   for (int c=0; c < queueSize; c++) {
      CellCoord *cc = &(queue[c]);

      // x edges
      for (int j = cc->y; j <= cc->y + 1; j++)
         for (int k = cc->z; k <= cc->z + 1; k++) {
            glvc(cc->x, j, k);
            glvc(cc->x + 1, j, k);
         }

      // y edges
      for (int i = cc->x; i <= cc->x + 1; i++)
         for (int k = cc->z; k <= cc->z + 1; k++) {
            glvc(i, cc->y, k);
            glvc(i, cc->y + 1, k);
         }

      // z edges
      for (int i = cc->x; i <= cc->x + 1; i++)
         for (int j = cc->y; j <= cc->y + 1; j++) {
            glvc(i, j, cc->z);
            glvc(i, j, cc->z + 1);
         }
   }

   glEnd(); // GL_LINES
   glPopAttrib();   
}

// draw a cylinder parallel to x, y, or z axis, from middle of cell <x1 y1 z1> to middle of cell <x2 y2 z2>.
void Maze3D::drawCylinder(int x1, int y1, int z1, int x2, int y2, int z2) {
   glPushMatrix();
   glTranslatef((x1 + 0.5) * maze.cellSize, (y1 + 0.5) * maze.cellSize, (z1 + 0.5) * maze.cellSize);
   // Figure out which axis cylinder is parallel to, and whether the direction is positive or negative,
   // so we can rotate appropriately.
   if (x1 != x2) glRotatef(x1 < x2 ? 90.0: -90.0, 0, 1, 0); // rotate 90 deg around y axis
   else if (y1 != y2) glRotatef(y1 < y2 ? -90.0 : 90.0, 1, 0, 0); // rotate 90 deg around x axis
   // else if (z1 < z2) // no rotation necessary
   else if (z2 < z1) glRotatef(180.0, 1, 0, 0); // rotate 180 deg around x or y axis

   //          quadric, baseRadius, topRadius,  height,   slices, stacks
   glColor3f(1.0, 1.0, 0.65);
   gluCylinder(cylQuadric, routeRadius, routeRadius, cellSize, 6, 1);
   glPopMatrix();
}

// draw lines along solution route
void Maze3D::drawSolutionRoute(void) {

#ifdef RouteWithLines
   // Here we set a constant line width.
   // Problem is that this is not perspective-correct: the width of a nearby line appears
   // the same as that of a far-off line.
   // Also, a line viewed nearly head-on won't have the depth cues that it would have if the nearer
   // end were thicker.
   // Since the solution route is mostly hidden inside the maze, the discrepancy shouldn't
   // be too bothersome.
   // But it would be worth trying as real objects (e.g. cylinders) to see how much better it looks.
   // The drawbacks there are (1) a bit harder to draw, since you have to figure out which axes to rotate around --
   // this can be fixed with a utility function; and (2) no easy stipple pattern -- but this could be addressed with
   // a texture, if necessary.
   glLineWidth(2.5);
   // yellowish lines
   glColor3f(1.0, 1.0, 0.7);
   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glEnable(GL_LINE_STIPPLE);
   // stipple pattern is increasing length of dashes toward exit: (space) 1 2 3 4; binary 0010110111011110
   // reverse that (opengl quirk) and add commas: 0111,1011,1011,0100. Hex: 0x7bb4
   glLineStipple(10, 0x7bb4);
   glBindTexture(GL_TEXTURE_2D, GL_NONE); // no texture
   glBegin(GL_LINE_STRIP); // using lines for now

#define midCell(c, ax) ((solutionRoute[c].ax + 0.5) * maze.cellSize)

   for (int c = 0; c < solutionRouteLen; c++) {
      glVertex3f(midCell(c, x), midCell(c, y), midCell(c, z));
   }

   glEnd(); // GL_LINES
   glPopAttrib();   

#else //RouteWithCylinders
   for (int c = 0; c < solutionRouteLen - 1; c++)
      drawCylinder(solutionRoute[c].x, solutionRoute[c].y, solutionRoute[c].z,
         solutionRoute[c+1].x, solutionRoute[c+1].y, solutionRoute[c+1].z);

//      glPushMatrix();
//      glTranslatef(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
//      // quadric, base, top, height, slices, stacks
//      gluCylinder(quadric, edgeRadius, edgeRadius, maze.cellSize, 4, 1);
//      glPopMatrix();

#endif // RouteWithLines/Cylinders

}


// Figure out the solution route. Use dead-end filler algorithm (http://www.astrolog.org/labyrnth/algrithm.htm).
// We assume that the maze is "perfect" (no loops).
void Maze3D::computeSolution(void) {
   // Initialize the isOnSolutionRoute member of each passage cell to 'true'.
   // We will then eliminate cells on dead-end passages until only the cells
   // really on the solution route are left.
   solutionRouteLen = 0;
   for (int i=0; i < w; i++)
      for (int j=0; j < h; j++)
         for (int k=0; k < d; k++)
            if (cells[i][j][k].state == Cell::passage) {
               cells[i][j][k].isOnSolutionRoute = true;
               solutionRouteLen++;
            }

   // debugMsg("solutionRouteLen (num of passage cells): %d\n", solutionRouteLen);

   bool somethingChanged;
   do {
      somethingChanged = false;
      // Loop through and find dead ends, and fill them (set isOnSolutionRoute = false) up to the next junction
      for (int i=0; i < w; i++)
         for (int j=0; j < h; j++)
            for (int k=0; k < d; k++) {
               int deadEndLen = fillInDeadEnd(i, j, k, NULL);
               if (deadEndLen > 0) {
                  // debugMsg("Found dead end of length %d at <%d, %d, %d>\n", deadEndLen, i, j, k);
                  solutionRouteLen -= deadEndLen;
                  somethingChanged = true;
               }
            }
   } while (somethingChanged);

   if (solutionRoute) delete [] solutionRoute;
   solutionRoute = new CellCoord[solutionRouteLen];

   // Fill in the remaining solution, placing each cell into solutionRoute in turn.
   int n = fillInDeadEnd(ccEntrance.x, ccEntrance.y, ccEntrance.z, solutionRoute);
   if (n != solutionRouteLen)
      errorMsg("n (%d) != solutionRouteLen (%d)\n", n, solutionRouteLen);

   return;
}

// If (x, y, z) is a dead end, fill it in (setting isOnSolutionRoute = false) and
// continue up the passage to the next junction.
// If route is null, don't fill in the entrance or exit cell.
// If route is non-null, store filled cells there in sequence.
// Return number of cells filled; this is zero if cell is not a dead end.
int Maze3D::fillInDeadEnd(int x, int y, int z, CellCoord *route) {
   // number of cells filled
   int c = 0;
   CellCoord ccCurr(x, y, z), ccNext;

   // debugMsg("fillInDeadEnd(%d, %d, %d): ", x, y, z);
   // if (route) debugMsg("(solution route): ");

   // Find out whether the current cell is passage and
   // has at most one neighbor that is passage and has isOnSolutionRoute = true.
   // (Don't fill in the entrance or exit cell, unless route pointer is non-null.)
   while (ccCurr.isCellPassage() && cells[ccCurr.x][ccCurr.y][ccCurr.z].isOnSolutionRoute
          && (route || !(ccCurr == ccEntrance || ccCurr == ccExit))
          && ccCurr.getSoleOpenNeighbor(ccNext)) {
      // If so, record current cell in route (if route record is requested)
      if (route) {
         route[c] = ccCurr;
      }
      // debugMsg("<%d, %d, %d> ", ccCurr.x, ccCurr.y, ccCurr.z);
      c++;
      // and fill in the current cell.
      cells[ccCurr.x][ccCurr.y][ccCurr.z].isOnSolutionRoute = false;

      // Then proceed to the neighbor.
      if (!ccNext.isInBounds()) break; // zero neighbors (as in case of exit), so nowhere to continue to
      else ccCurr = ccNext;
   }

   // debugMsg("\n");

   return c;
}

extern int level;

void Maze3D::addPrizeAt(CellCoord &cc) {
   prizes[nPrizes].taken = false;
   prizes[nPrizes].where = cc;
   cells[cc.x][cc.y][cc.z].iPrize = nPrizes;
   nPrizes++;
}

// Populate the maze with prize objects.
void Maze3D::addPrizes() {
   int goal = level;
   if (goal > prizeMax) goal = prizeMax;

   CellCoord ccTmp(0,0,0);

   for (int i = 0; i < goal; i++) {
      int attempts = 0;
      bool validPlace = false;
      do {
         ccTmp.placeRandomly();
//         debugMsg("placing prize %d at <%d %d %d>: ", i, ccTmp.x, ccTmp.y, ccTmp.z);
         if (!ccTmp.isCellPassage()) ; // debugMsg(" !ccTmp.isCellPassage()\n");
         else if (ccTmp == ccEntrance) ; // debugMsg(" ccTmp == ccEntrance\n");
         else if (ccTmp == ccExit) ; // debugMsg(" ccTmp == ccExit\n");
         else if (cells[ccTmp.x][ccTmp.y][ccTmp.z].iPrize != -1) ; // debugMsg(" already has prize %d\n", cells[ccTmp.x][ccTmp.y][ccTmp.z].iPrize);
         else {
            validPlace = true;
//          debugMsg("ok!\n");
         }

         if (!validPlace && ++attempts > 150) {
            debugMsg("Too many attempts to place prize %d; giving up.\n", i);
            break; // this will exit only the do loop; don't place the prize in an invalid spot.
         }
      } while (!validPlace);

      if (validPlace)
         addPrizeAt(ccTmp);
         //prizes[i].taken = false;
         //prizes[i].where = ccTmp;
         //cells[ccTmp.x][ccTmp.y][ccTmp.z].iPrize = i;
   }

   // debugging: are prizes[*].where really separate objects, as they should be? yes
   //for (int i=0; i < nPrizes; i++)
   //   debugMsg("prize %d at %d %d %d\n", i, prizes[i].where.x, prizes[i].where.y, prizes[i].where.z);

   nPrizesLeft = nPrizes;
   return;
}

void Maze3D::addPictureAt(CellCoord &cc, Wall *w, char dir) {
   Picture *p = &pictures[nPictures];
   p->where = cc;
   p->wall = w;
   p->textureId = pictureTexture;
   p->dir = dir;
   p->setupVertices();

   cells[cc.x][cc.y][cc.z].picture = p;
   nPictures++;
}

// Populate the maze with prize objects.
void Maze3D::addPictures() {
   int goal = numPassageCells / pictureRarity;
   if (goal > pictureMax) goal = pictureMax;
   debugMsg("picture goal: %d\n", goal);

   CellCoord ccTmp(0,0,0), nc(0,0,0);

   for (int i = 0; i < goal; i++) {
      int attempts = 0;
      bool validPlace = false;
      Wall *wall;
      do {
         ccTmp.placeRandomly();
//         debugMsg("placing prize %d at <%d %d %d>: ", i, ccTmp.x, ccTmp.y, ccTmp.z);
         if (!ccTmp.isCellPassage()) ; // debugMsg("picture: !ccTmp.isCellPassage()\n");
         else if (ccTmp == ccEntrance) ; // debugMsg("picture: ccTmp == ccEntrance\n");
         else if (ccTmp == ccExit) ; // debugMsg("picture: ccTmp == ccExit\n");
         else if (cells[ccTmp.x][ccTmp.y][ccTmp.z].picture) ; // debugMsg("has a picture already\n");
         else {
            nc = ccTmp;
            nc.x++; //##TODO: randomize direction, or better, try all four (six) in random order
            wall = ccTmp.findWallTo(&nc);
            if (wall->state == Wall::CLOSED && !wall->seeThrough) {
               validPlace = true;
               debugMsg("picture: ok!\n");
            }
         }

         if (!validPlace && ++attempts > 150) {
            debugMsg("Too many attempts to place picture %d; giving up.\n", i);
            break; // this will exit only the do loop; don't place the picture in an invalid spot.
         }
      } while (!validPlace);

      if (validPlace)
         addPictureAt(ccTmp, wall, 'x');
   }

   // debugging: are pictures[*].where really separate objects, as they should be? yes
   //for (int i=0; i < nPictures; i++)
   //   debugMsg("picture %d at %d %d %d\n", i, pictures[i].where.x, pictures[i].where.y, pictures[i].where.z);

   return;
}

#ifdef DEBUGGING
static int numTimes = 0;
static const int maxNumTimes = 500;
#endif // DEBUGGING

// draw the prizes (that are not taken).
void Maze3D::drawPrizes() {
   glLineWidth(1.0); // for outline. 1 is default anyway.

   const float throbRate = 0.1, throbSize = 0.05, baseRadius = 0.1;

   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

#ifdef DEBUGGING
   //numTimes++;
   //if (numTimes < maxNumTimes)
   //   debugMsg("Drawing prizes: \n");
#endif // DEBUGGING
   for (int i=0; i < nPrizes; i++) {
      if (!prizes[i].taken) {
         CellCoord *cc = &(prizes[i].where);
#ifdef DEBUGGING
         //if (numTimes < maxNumTimes)
         //   debugMsg("prize %d at %d %d %d (%staken)\n", i, cc->x, cc->y, cc->z, prizes[i].taken ? "" : "not ");
#endif // DEBUGGING
         glPushMatrix();
         glTranslatef((cc->x + 0.5) * maze.cellSize, (cc->y + 0.5) * maze.cellSize, (cc->z + 0.5) * maze.cellSize);
         // spin. Add in x y z so that prizes aren't all in phase with each other.
         float animFactor = (maze.exitRot + cc->x*30 + cc->y*41 + cc->z*52) * 0.5;
         float radius = baseRadius * maze.cellSize * (1.0 + throbSize * sin(animFactor * throbRate));
         // vary the spin directions too, based on which cell we're in.
         glRotatef((cc->x + cc->y) % 2 ? animFactor : -animFactor, 0.0f, 0.0f, 1.0f);
         glRotatef((cc->x + cc->z) % 2 ? animFactor : -animFactor, 0.0f, 1.0f, 0.0f);
         // draw faces of sphere
         gluQuadricDrawStyle(sphereQuadric, GLU_FILL);
         glColor3f(1.0, 1.0, 1.0); // white
         // quadric, radius, slices, stacks
         gluSphere(sphereQuadric, radius, 9, 9);
         // draw outline of sphere?
         gluQuadricDrawStyle(sphereQuadric, GLU_LINE);
         glColor3f(0.1, 0.1, 1.0); // blue
         // quadric, radius, slices, stacks
         gluSphere(sphereQuadric, radius, 9, 9);
         glPopMatrix();
      }
   }

   glPopAttrib();
}

// draw wall pictures
void Maze3D::drawPictures() {

   glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
   glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   for (int i=0; i < nPictures; i++) {
      pictures[i].draw();
   }

   glPopAttrib();
}
