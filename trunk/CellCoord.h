#ifndef __CellCoord_h__
#define __CellCoord_h__

#include <math.h>

#include "Cell.h"
#include "Wall.h"
#include "glPoint.h"

class CellCoord {
public:
	int x, y, z;
	CellCoord(int a=0, int b=0, int c=0) { x = a, y = b, z = c; };
	inline boolean operator ==(CellCoord &nc) { return (x == nc.x && y == nc.y && z == nc.z); }
        bool isCellPassage(void);
	bool isCellPassageSafe(void);
	void setCellState(Cell::CellState v);
	Cell::CellState getCellState();
	void init(glPoint &p);

	// distance squared (euclidean)
	inline int dist2(int i, int j, int k) { return (x - i)*(x - i) + (y - j)*(y - j) + (z - k)*(z - k); }
	// manhattan distance
	inline int manhdist(CellCoord &nc) { return abs(x - nc.x) + abs(y - nc.y) + abs(z - nc.z); }
	/* Return true iff this cell is a legal place to expand into from fromCC.
		Assumes both are within bounds of maze grid, that they differ in only one axis and
		by only one unit, that fromCC is already passage, and that this cell is uninitialized.
		Checks whether this cell is too close to other passages, violating sparsity. */
	bool isCellPassable(CellCoord *fromCC);
	
	/* Find walls[] array element of wall between this and nc.
	 * Assumes cell coordinates of this and nc differ in only one axis and by at most one unit.
	 * Also assumes the higher of all coord pairs is within bounds 0 <= coord <= w/h/d
	 * (i.e. one extra on the top is ok). E.g. if coords differ in x, then the higher of x and nc->x
	 * must be 0 <= x <= w. */
	Wall *findWallTo(CellCoord *nc);
	/* Set wall between this and nc to given state.
	 * See assumptions of findWallTo().
	 * Also assumes this is "inside" and nc is "outside", if it matters
	 (this assumption can be incorrect if sparseness <= 1). */
	void setStateOfWallTo(CellCoord *nc, Wall::WallState state);
	/* Set wall of this cell in direction dx,dy,dz to given state.
	 * See assumptions of findWallTo().
	 * Also assumes this is "inside" and cell in dx,dy,dz is "outside", if it matters
	 (this assumption can be incorrect if sparseness <= 1). */
	void setStateOfWallTo(int dx, int dy, int dz, Wall::WallState state);
	// If the wall in direction dx,dy,dz is closed, open it and return wall.
	// Else return null.
	inline Wall *tryOpenWall(int dx, int dy, int dz);

	/* Open a wall of this cell that is not already open. Prefer vertical walls. */
	// Return chosen wall.
	// ## To do: don't assert
	Wall *openAWall();
	/* Set the camera on the other side of wall w from this cc, facing this cc. */
	void standOutside(Wall *w);

#if 1
	/* Get state of wall between this and nc.
	* See assumptions of findWallTo(). */
	inline Wall::WallState CellCoord::getStateOfWallTo(CellCoord *nc) {
		Wall *w = findWallTo(nc);
		return w->state;
	}
	/* Get wall of this cell in direction dx,dy,dz.
		* See assumptions of findWallTo(). */
	inline Wall::WallState CellCoord::getStateOfWallTo(int dx, int dy, int dz) {
		static CellCoord nc;
		nc.x = x+dx; nc.y = y+dy; nc.z = z+dz;
		return getStateOfWallTo(&nc);
	}
#else // 0
	/* Get state of wall between this and nc.
	 * See assumptions of findWallTo(). */
	Wall::WallState getStateOfWallTo(CellCoord *nc);
	/* Get wall of this cell in direction dx,dy,dz.
	 * See assumptions of findWallTo(). */
	inline Wall::WallState getStateOfWallTo(int dx, int dy, int dz);
#endif
	/* Get state of wall between this and nc.
	 * Checks boundaries: does not assume coords are in range. */
	Wall::WallState getStateOfWallToSafe(CellCoord *nc);

	// Convenience function for Wall::drawExit()
	inline void drawExit(Wall *w, bool isEntrance) {
		w->drawExit(x, y, z, isEntrance);
	}

	/* Randomly shuffle the order of CellCoords in an array.
	 * Ref: http://en.wikipedia.org/wiki/Fisher-Yates_shuffle#The_modern_algorithm */
	static void shuffleCCs(CellCoord *ccs, int n);

        // Find out whether this cell has only one "open" neighbor (i.e. that is passage and has isOnSolutionRoute = true).
        // If so, populate position of neighbor and return true.
        bool getSoleOpenNeighbor(CellCoord &neighbor);

        // Return true if this cc's coords are inside the maze bounds.
        bool isInBounds(void);

        // Set coordinates to random value within bounding box of maze.
        void placeRandomly(void);

private:
        // Check whether neighbor cell at this + (dx, dy, dz) is open
        // (i.e. that is passage and has isOnSolutionRoute = true).
        // If so, put neighbor cell's coords into neighbor and return true.
        bool checkNeighborOpen(int dx, int dy, int dz, CellCoord &neighbor);

};

extern CellCoord ccExit, ccEntrance;

#endif // __CellCoord_h__
