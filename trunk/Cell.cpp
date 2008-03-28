#include "Cell.h"
#include "Maze3D.h"

Cell::CellState Cell::getCellState(int x, int y, int z) { return maze.cells[x][y][z].state; };
