#include "CellCoord.h"
#include "Maze3D.h"
#include "maze3dflyer.h"

void CellCoord::setCellState(Cell::CellState v) { maze.cells[x][y][z].state = v; }

bool CellCoord::isCellPassage() { return maze.cells[x][y][z].state == Cell::passage; }

Cell::CellState CellCoord::getCellState() { return maze.cells[x][y][z].state; }

void CellCoord::init(glPoint &p) {
	x = int(floor(p.x / Maze3D::cellSize));
	y = int(floor(p.y / Maze3D::cellSize));
	z = int(floor(p.z / Maze3D::cellSize));
}

/* Return true iff this cell is a legal place to expand into from fromCC.
	Assumes both are within bounds of maze grid, that they differ in only one axis and
	by only one unit, that fromCC is already passage, and that this cell is uninitialized.
	Checks whether this cell is too close to other passages, violating sparsity. */
bool CellCoord::isCellPassable(CellCoord *fromCC) {
	/* Trivial case: if sparsity == 1, no check for closeness. */
	if (maze.sparsity <= 1) return true;

	// get direction of movement
	int dx = x - fromCC->x, dy = y - fromCC->y, dz = z - fromCC->z;
	// compute bounding box to check: lower & upper bounds for i, j, k ####
	int i, j, k, il, iu, jl, ju, kl, ku;
	// radius squared
	int sparsity2 = maze.sparsity * maze.sparsity;
	il = x - maze.sparsity + 1;
	iu = x + maze.sparsity - 1;
	jl = y - maze.sparsity + 1;
	ju = y + maze.sparsity - 1;
	kl = z - maze.sparsity + 1;
	ku = z + maze.sparsity - 1;
	if (dx > 0) il = x;
	else if (dx < 0) iu = x;
	else if (dy > 0) jl = y;
	else if (dy < 0) ju = y;
	else if (dz > 0) kl = z;
	else /* dz < 0 */ ku = z;

	//debugMsg("isCellPassable(): %d,%d,%d from %d,%d,%d; bounds: %d,%d,%d to %d,%d,%d\n",
	//	x, y, z, fromCC->x, fromCC->y, fromCC->z, il,jl,kl, iu,ju,ku);

	// trim bbox to maze grid
	il = max(il, 0);
	jl = max(jl, 0);
	kl = max(kl, 0);
	iu = min(iu, maze.w-1);
	ju = min(ju, maze.h-1);
	ku = min(ku, maze.d-1);

	// Iterate through bbox, looking for already-occupied cells within radius.
	// Note, even when ijk = xyz, we don't reject ourselves because the current cell is not passage.
	for (i = il; i <= iu; i++)
		for (j = jl; j <= ju; j++)
			for (k = kl; k <= ku; k++)
				if (Cell::getCellState(i, j, k) == Cell::passage &&
					dist2(i, j, k) < sparsity2) {
					//debugMsg("   blocked at %d,%d,%d, dist2 = %d\n", i, j, k, dist2(i, j, k));
					return false;
				}
	return true;
}

/* Find walls[] array element of wall between this and nc.
	* Assumes cell coordinates of this and nc differ in only one axis and by at most one unit.
	* Also assumes the higher of all coord pairs is within bounds 0 <= coord <= w/h/d
	* (i.e. one extra on the top is ok). E.g. if coords differ in x, then the higher of x and nc->x
	* must be 0 <= x <= w. */
Wall *CellCoord::findWallTo(CellCoord *nc) {
	Wall *w;
	if (nc->x != x)	{
		w = &(maze.xWalls[(nc->x > x) ? nc->x : x][y][z]);
		//debugMsg("   found xWalls[%d %d %d]\n", ((nc->x > x) ? nc->x : x), y, z);
	}
	else if (nc->y != y) {
		w = &(maze.yWalls[x][(nc->y > y) ? nc->y : y][z]);
		//debugMsg("   found yWalls[%d %d %d]\n", x, ((nc->y > y) ? nc->y : y), z);
	}
	else {
		w = &(maze.zWalls[x][y][(nc->z > z) ? nc->z : z]);
		//debugMsg("   found zWalls[%d %d %d]\n", x, y, ((nc->z > z) ? nc->z : z));
	}
	return w;
}

/* Set wall between this and nc to given state.
	* See assumptions of findWallTo().
	* Also assumes this is "inside" and nc is "outside", if it matters
	(this assumption can be incorrect if sparseness <= 1). */
void CellCoord::setStateOfWallTo(CellCoord *nc, Wall::WallState state) {
	Wall *w = findWallTo(nc);
	w->state = state;
	if (state == Wall::CLOSED) {
		w->outsidePositive = (nc->x - x + nc->y - y + nc->z - z > 0);
		//debugMsg("  %d %d %d.setWallTo(%d %d %d, %d): op=%c\n", x, y, z, nc->x, nc->y, nc->z,
		//	state, w->outsidePositive ? 'y' : 'n');
	}
}

/* Set wall of this cell in direction dx,dy,dz to given state.
	* See assumptions of findWallTo().
	* Also assumes this is "inside" and cell in dx,dy,dz is "outside", if it matters
	(this assumption can be incorrect if sparseness <= 1). */
void CellCoord::setStateOfWallTo(int dx, int dy, int dz, Wall::WallState state) {
	static CellCoord nc;
	nc.x = x+dx; nc.y = y+dy; nc.z = z+dz;
	//nc.x += dx; nc.y += dy; nc.z += dz;
	Wall *w = findWallTo(&nc);
	w->state = state;
	if (state == Wall::CLOSED) {
		w->outsidePositive = (dx + dy + dz > 0);
		//debugMsg("  %d %d %d.setWallTo(%d %d %d, %d): op=%c\n", x, y, z, nc.x, nc.y, nc.z,
		//	state, w->outsidePositive ? 'y' : 'n');
	}
}

// If the wall in direction dx,dy,dz is closed, open it and return wall.
// Else return null.
inline Wall *CellCoord::tryOpenWall(int dx, int dy, int dz) {
	static CellCoord nc;
	nc.x = x+dx; nc.y = y+dy; nc.z = z+dz;
	//nc.x += dx; nc.y += dy; nc.z += dz;
	Wall *w = findWallTo(&nc);
	
	if (w->state == Wall::CLOSED) {
		w->state = Wall::OPEN;
		return w;
	}
	else return (Wall *)NULL;
}

/* Open a wall of this cell that is not already open. Prefer vertical walls. */
// Return chosen wall.
// ## To do: don't assert
Wall *CellCoord::openAWall() {
	Wall *w;
	// Should maybe vary this order?
	w = tryOpenWall(-1, 0, 0); if (w) return w;
	w = tryOpenWall(0, 0, -1); if (w) return w;
	w = tryOpenWall(0, 0,  1); if (w) return w;
	w = tryOpenWall( 1, 0, 0); if (w) return w;
	w = tryOpenWall(0, -1, 0); if (w) return w;
	w = tryOpenWall(0,  1, 0); if (w) return w;

	errorMsg("Entrance or exit %d,%d,%d has no closed walls to open.\n", x, y, z);
	return (Wall *)NULL;
}

/* Set the camera on the other side of wall w from this cc, facing this cc. */
void CellCoord::standOutside(Wall *w) {
	GLfloat cx = (x + 0.5f) * Maze3D::cellSize, cy = (y + 0.5f) * Maze3D::cellSize, cz = (z + 0.5f) * Maze3D::cellSize;
	GLfloat heading = 0.0f, pitch = 0.0f;
	Vertex *qv = w->quad.vertices;

	debugMsg("standOutside: cc(%d, %d, %d), w(%2.2f,%2.2f,%2.2f / %2.2f,%2.2f,%2.2f): ",
		x, y, z, qv[0].x, qv[0].y, qv[0].z, qv[2].x, qv[2].y, qv[2].z);
	if (qv[0].x == qv[2].x) { // If wall is in X plane
		debugMsg("x plane;\n");
		heading = qv[0].x > cx ? -90.0f : 90.0f;
		// pitch = 0;
		cx += (qv[0].x - cx) * 2;
	} else if (qv[0].y == qv[2].y) { // If wall is in Y plane
		debugMsg("y plane;\n");
		pitch = qv[0].y > cy ? Cam.m_MaxPitch : -Cam.m_MaxPitch;
		// heading = 0;
		cy += (qv[0].y - cy) * 2;
	} else if (qv[0].z == qv[2].z) { // If wall is in z plane
		debugMsg("z plane;\n");
		heading = qv[0].z > cz ? 0.0f : 180.0f;
		// pitch = 0;
		cz += (qv[0].z - cz) * 2;
	}

	debugMsg("  GoTo(%2.2f,%2.2f,%2.2f, p %2.2f, h %2.2f)\n",
		cx, cy, cz, pitch, heading);
	Cam.GoTo(cx, cy, -cz, pitch, heading);
	Cam.AccelForward(-keyAccelRate*2);
}

/* Get state of wall between this and nc.
	* Checks boundaries: does not assume coords are in range. */
Wall::WallState CellCoord::getStateOfWallToSafe(CellCoord *nc) {
	//debugMsg(" gSOWTS(%d, %d, %d) ", nc->x, nc->y, nc->z);
	// Is this right?
	if ((x >= maze.w && nc->x >= maze.w) ||
		(y >= maze.h && nc->y >= maze.h) ||
		(z >= maze.d && nc->z >= maze.d) ||
		(x < 0 && nc->x < 0) ||
		(y < 0 && nc->y < 0) ||
		(z < 0 && nc->z < 0))
	{ // debugMsg(" ob1 ");
	return Wall::OPEN; }
	if (x > maze.w || nc->x > maze.w ||
		y > maze.h || nc->y > maze.h ||
		z > maze.d || nc->z > maze.d ||
		x < -1 || nc->x < -1 ||
		y < -1 || nc->y < -1 ||
		z < -1 || nc->z < -1)
	{ // debugMsg(" ob2 ");
	return Wall::OPEN; }

	Wall *w = findWallTo(nc);
	//debugMsg(" wstate: %d ", w->state);
	return w->state;
}

/* Randomly shuffle the order of CellCoords in an array.
	* Ref: http://en.wikipedia.org/wiki/Fisher-Yates_shuffle#The_modern_algorithm */
void CellCoord::shuffleCCs(CellCoord *ccs, int n) {
	CellCoord tmp;
	int k;
	for (; n >= 2; n--) {
		k = rand() % n;
		if (k != n-1) {
			tmp = ccs[k];
			ccs[k] = ccs[n-1];
			ccs[n-1] = tmp;
		}
	}
}

extern CellCoord ccExit, ccEntrance;
