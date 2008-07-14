#include "Maze3D.h"

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

// setDim should be called only before SetupWorld().
void Maze3D::setDim(int _w, int _h, int _d, int _s, int _b) {
   w = _w, h = _h, d = _d;
   sparsity = _s;
   branchClustering = _b;
}

bool Maze3D::IsInside(glPoint p) {
   CellCoord cc; //TODO: use a static one
   //TODO: should this function be a member of CellCoord?
   p.z = -p.z;
   cc.init(p);
   return (cc.isCellPassageSafe());
   //    return (maze.cells[x][y][z].state == Cell::passage);
}

