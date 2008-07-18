#include "Maze3D.h"
#include "maze3dflyer.h"

// Two different ways to draw outline. Define exactly one.
#define OutlineWithLines 1
// I don't recommend cylinders. At close range, they look like crude cylinders
// (square prisms). At far range they look like dotted lines.
// #define OutlineWithCylinders 1

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
   if (whichAxis < w + h) d++;
   else if (whichAxis < w + h + w + d) h++;
   else w++;

   int minDim = min(w, min(h, d));
   if (minDim < 3) sparsity = 2;
   else {
      // This is my pseudoscientific, lazy attempt at approximating a normal distribution.
      // r is a number in [0,1]
      float r = ((rand() % 98) + (rand() % 99) + 0.0) / 195.0;
      // bias against the highest number (minDim) by subtracting an extra 0.2.
      // round the result by adding 0.5 and casting to int.
      sparsity = int(2.0 + (minDim - 2.2) * r + 0.5);
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
void Maze3D::drawXEdge(int i, int j, int k) {
   //FIXME: there are missing edges and spurious edges along outer planes
   bool jplus, jminus, kplus, kminus;
   // are xy walls in +y/-y directions closed?
   jplus  = j < h && IsWall(&zWalls[i][j  ][k]);
   jminus = j > 0 && IsWall(&zWalls[i][j-1][k]);;
   // are xz walls in +z/-z directions closed?
   kplus  = k < d && IsWall(&yWalls[i][j][k  ]);
   kminus = k > 0 && IsWall(&yWalls[i][j][k-1]);

   // For now, just draw outlines for external edges
   //TODO: consider internal edges too

   //// debugging:
   //if (firstTime) {
   //   debugMsg("drawXEdge(%d %d %d) - [%s %s %s %s]: %d\n", i, j, k,
   //      jplus  ? "j+" : "",
   //      jminus ? "j-" : "",
   //      kplus  ? "k+" : "",
   //      kminus ? "k-" : ""
   //      );
   //}

   // draw an edge only if there are two walls at right angles to e.o.
   if ((jplus || jminus) && (kplus || kminus)) {
      // For now, we'll try lines, not quads; default width (prefer 1 pixel)
      // glColor3f(1.0, 0.0, 0.0); // red - debugging
      glVertex3f(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
      glVertex3f((i + 1) * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
   }
}

// Draw an outline edge that is parallel to the Y axis, if it ought to be drawn.
// Assumes j < h.
void Maze3D::drawYEdge(int i, int j, int k) {
   bool iplus, iminus, kplus, kminus;
   // are xy walls in +x/-x directions closed?
   iplus  = i < w && IsWall(&zWalls[i  ][j][k]);
   iminus = i > 0 && IsWall(&zWalls[i-1][j][k]);
   // are yz walls in +z/-z directions closed?
   kplus  = k < d && IsWall(&xWalls[i][j][k  ]);
   kminus = k > 0 && IsWall(&xWalls[i][j][k-1]);

   // draw an edge only if there are two walls at right angles to e.o.
   if ((iplus || iminus) && (kplus || kminus)) {
      // glColor3f(0.0, 1.0, 0.0); // green - debugging
      glVertex3f(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
      glVertex3f(i * maze.cellSize, (j + 1) * maze.cellSize, k * maze.cellSize);
   }
}

// Draw an outline edge that is parallel to the Z axis, if it ought to be drawn.
// Assumes k < d.
void Maze3D::drawZEdge(int i, int j, int k) {
   bool iplus, iminus, jplus, jminus;
   // are xz walls in +x/-x directions closed?
   iplus  = i < w && IsWall(&yWalls[i  ][j][k]);
   iminus = i > 0 && IsWall(&yWalls[i-1][j][k]);
   // are yz walls in +y/-y directions closed?
   jplus  = j < h && IsWall(&xWalls[i][j  ][k]);
   jminus = j > 0 && IsWall(&xWalls[i][j-1][k]);

   // draw an edge only if there are two walls at right angles to e.o.
   if ((iplus || iminus) && (jplus || jminus)) {
      // glColor3f(0.0, 0.0, 1.0); // blue - debugging
#ifdef OutlineWithLines
      glVertex3f(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
      glVertex3f(i * maze.cellSize, j * maze.cellSize, (k + 1) * maze.cellSize);
#else
      // quadric, base, top, height, slices, stacks
      glPushMatrix();
      glTranslatef(i * maze.cellSize, j * maze.cellSize, k * maze.cellSize);
      gluCylinder(quadric, edgeRadius, edgeRadius, maze.cellSize, 4, 1);
      glPopMatrix();
#endif // OutlineWithLines

   }
}

// draw dark edges along outlines
void Maze3D::drawOutline(void) {
   // for smooth lines, need to turn on blending.
   // see http://www.opengl.org/resources/faq/technical/rasterization.htm#rast0150
   glColor3f(0.3, 0.3, 0.3);
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
