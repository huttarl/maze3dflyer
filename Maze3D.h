#ifndef __Maze3D_h__
#define __Maze3D_h__

#include <ctime> //FIXME: is this portable?

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
        // Number of cells, after maze generation, that are passage.
        int numPassageCells;
	// size of one cell in world coordinates. Some code might depend on cellSize = 1.0
	const static float cellSize;
	// margin around walls that we can't collide with; should be << cellSize.
	const static float wallMargin;
	static bool checkCollisions;
        static clock_t whenEntered, whenSolved, lastSolvedTime;
        static bool hasFoundExit, newBest;
	static float exitRot; // used for rotating exit decorations
        static float exitHoleRadius;
        static float exitThickness;
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

        // Maze3D::Maze3D();
        Maze3D::Maze3D(int _w = 8, int _h = 8, int _d = 8, int _s = 3, int _b = 2);
        void Maze3D::setDim(int _w = 8, int _h = 8, int _d = 8, int _s = 3, int _b = 2);

        bool Maze3D::IsInside(glPoint position);
};

extern Maze3D maze;

#endif // __Maze3D_h__
