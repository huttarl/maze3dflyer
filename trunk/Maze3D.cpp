#include "Maze3D.h"

// size of one cell in world coordinates. Some code may depend on cellSize = 1.0
const float Maze3D::cellSize = 1.0f;
// margin around walls that we can't collide with; should be << cellSize.
const float Maze3D::wallMargin = 0.15f;
// apparently these statics have to be defined outside of the class def.
float Maze3D::exitRot = 0.0f;
float Maze3D::exitHoleRadius = 0.3f;
float Maze3D::exitThickness = Maze3D::cellSize * 0.07;
bool Maze3D::checkCollisions = true;
bool Maze3D::hasEntered = false;
bool Maze3D::hasExited = false;
