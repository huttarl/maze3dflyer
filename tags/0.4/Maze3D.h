#ifndef __Maze3D_h__
#define __Maze3D_h__

#include "Cell.h"
#include "CellCoord.h"
#include "Wall.h"

class Maze3D {
public:
	const static int wMax = 20, hMax = 20, dMax = 20;	// This is necessary because C++ doesn't do fully dynamic
												// multidimensional arrays. I've been spoiled by Java.

	// The following probably belong in a Maze class.
	int w, h, d;			// width (x) of maze in cells
	// sparsity: two passageway cells cannot be closer to each other than sparsity, unless
	// they are connected as directly as possible.
	int sparsity;	// how sparse the maze must be
	// branchClustering affects tendency for the maze to branch out as much as possible from any given cell.
	// The maze will branch out as much as possible up to branchClustering branches; after that, the
	// likelihood decreases. Suggested range 1-6.
	int branchClustering;
	// size of one cell in world coordinates. Some code may depend on cellSize = 1.0
	const static float cellSize;
	// margin around walls that we can't collide with; should be << cellSize.
	const static float wallMargin;
	static bool checkCollisions;
	static float exitRot;
	/* The maze consists of both cells and walls. Each wall may be shared by two cells. */
	Cell (*cells)[hMax][dMax];		// The maze's cells
	CellCoord ccExit, ccEntrance;
	/* C++ doesn't support dynamic multidimensional arrays very well, hence the following cruft. */
	// The maze's walls
	Wall (*xWalls)[Maze3D::hMax][Maze3D::dMax];    // Walls facing along X axis   [w+1][h][d]
	Wall (*yWalls)[Maze3D::hMax+1][Maze3D::dMax]; // Walls facing along Y axis   [w][h+1][d]
	Wall (*zWalls)[Maze3D::hMax][Maze3D::dMax+1];  // Walls facing along Z axis   [w][h][d+1]
	// Pointers to the open "walls" at entrance and exit.
	Wall *exitWall, *entranceWall;

	Maze3D::Maze3D() {
		w = 8, h = 8, d = 8;
		sparsity = 3;
		branchClustering = 2;
		//cellSize = 1.0f;
		//wallMargin = 0.11f;
		checkCollisions = true;
		exitRot = 0.0f;
	}
};

extern Maze3D maze;

#endif // __Maze3D_h__