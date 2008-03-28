/*
 *		maze3dflyer (c) by Lars Huttar, 2007-2008  http://www.huttar.net
 *		Project home page: http://code.google.com/p/maze3dflyer

 *		Based on NeHe lesson10, by Lionel Brits & Jeff Molofee 2000
 *		nehe.gamedev.net
 *		Conversion to Visual Studio.NET done by GRANT JAMES(ZEUS)"

 *		Also uses quaternion camera class by Vic Hollis:
 *		http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=Quaternion_Camera_Class
 *		JPG loading code (may not be used) by Ronny André Reierstad from APRON tutorials at
 *			http://www.morrowland.com/apron/tut_gl.php
 */

/*
 * Terminology note:
 * I have sometimes used the following terms interchangeably: visited (of a cell), passage, open, occupied, carved
 * But after algorithm changes, a cell having been "visited" (by the algorithm) no longer implies that it is "passage",
 * as cells that are not part of the navigable maze can now exist.
 */

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <new>
#include <assert.h>

#include <gl/gl.h>			// OpenGL header files
#include <gl/glu.h>			// 
// #include <gl/glaux.h>	// unneeded, apparently
#include <gl/glut.h>		// (glut.h actually includes gl and glu.h, so we're redundant)

#include "glCamera.h"

glCamera Cam;				// Our Camera for moving around and setting prespective
							// on things.

#define DEBUGGING 1
//#ifdef DEBUGGING
//#define USE_FONTS 1
//#endif

HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

#ifdef USE_FONTS
GLuint	base;				// Base Display List For The Font Set
#endif

bool	keysDown[256];		// keys for which we have received down events (and no release)
bool	keysStillDown[256];	// keys for which we have already processed inital down events (and no release)
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=TRUE;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	blend=FALSE;		// Blending ON/OFF
bool	autopilot=TRUE;		// Autopilot on?
bool	mouseGrab=FALSE;		// mouse centering is on?
bool	aaPolys = FALSE;	// antialias polygons?
//bool	bp=FALSE;			// B pressed and not yet released?
//bool	fp=FALSE;			// F pressed and not yet released?
//bool	pp=FALSE;			// P Pressed and not yet released?
//bool	mp=FALSE;			// M pressed and not yet released?
//bool	cp=FALSE;			// C pressed and not yet released?

float	keyTurnRate = 2.0f; // how fast to turn in response to keys
float	keyAccelRate = 0.1f; // how fast to accelerate in response to keys
float	keyMoveRate = 0.1f;  // how fast to move in response to keys

// Used for mouse steering
UINT	MouseX, MouseY;		// Coordinates for the mouse
UINT	CenterX, CenterY;	// Coordinates for the center of the screen.

void CheckMouse(void);
bool CheckKeys(void);

const float piover180 = 0.0174532925f;
static int xRes = 800;
static int yRes = 600;
static char title[] = "3D Maze Flyer";

GLfloat xheading = 0.0f, yheading = 0.0f;
GLfloat xpos = 0.5f, ypos = 0.5f, zpos = 10.0f;
GLUquadricObj *quadric;	

const numFilters = 3;		// How many filters
GLuint	filter;				// Which Filter To Use
typedef enum { brick, carpet, wood, popcorn } Material;
const numTextures = (popcorn - brick + 1);		// Max # textures (not counting filtering)
GLuint	textures[numFilters * numTextures];		// Storage for 4 texture indexes, with 3 filters each.


const int wMax = 20, hMax = 20, dMax = 20; // This is necessary because C++ doesn't do fully dynamic
							// multidimensional arrays. I've been spoiled by Java.
// The following probably belong in a Maze class.
static int w = 7;			// width (x) of maze in cells
static int h = 10;			// height (y) of maze in cells
static int d = 10;			// depth (z) of maze in cells
// sparsity: two passageway cells cannot be closer to each other than sparsity, unless
// they are connected as directly as possible.
static int sparsity = 3;	// how sparse the maze must be
// branchClustering affects tendency for the maze to branch out as much as possible from any given cell.
// The maze will branch out as much as possible up to branchClustering branches; after that, the
// likelihood decreases. Suggested range 1-6.
static int branchClustering = 2;
// size of one cell in world coordinates. Some code may depend on cellSize = 1.0
static const float cellSize = 1.0f;
// margin around walls that we can't collide with; should be << cellSize.
static const float wallMargin = 0.11f;
// distance from eye to near clipping plane. Should be about wallMargin/2.
static const float zNear = (wallMargin * 0.6f);
// distance from eye to far clipping plane.
static const float zFar = (cellSize * 100.0f);
static bool checkCollisions = true;

// Maze::collide?
/* Return true iff moving from point p along vector v would collide with a wall. */
extern bool collide(glPoint &p, glVector &v);

typedef struct tagVERTEX
{
	float x, y, z;
	float u, v;
} Vertex;

typedef struct tagQUAD
{
	Vertex vertices[4];
} Quad;

void debugMsg(const char *str, ...)
{
 char buf[2048];

 va_list ptr;
 va_start(ptr,str);
 vsprintf(buf,str,ptr);

 OutputDebugString(buf);
}

// To do: do something different with error messages than debug messages.
void errorMsg(const char *str, ...)
{
 char buf[2048];

 va_list ptr;
 va_start(ptr,str);
 vsprintf(buf,str,ptr);

 OutputDebugString("\nError: ");
 OutputDebugString(buf);
}


// class instance counters
static int nic=0; // CellCoord
static int nc=0; // Cell
static int nw=0; // Wall
static bool firstTime = true; // for debugging

static GLfloat exitRot = 0.0f;

class Wall {
public:
	enum WallState { UNINITIALIZED, OPEN, CLOSED } state;
	// outsidePositive: true if "outside" of face is in positive axis direction.
	// only applies to CLOSED walls.
	bool outsidePositive;
	Quad quad;
	Wall() { state = UNINITIALIZED; outsidePositive = false; }
	// Within a glBegin/glEnd, draw a quad for this wall.
	void draw(char dir) {
		switch (dir) {
			case 'x': glNormal3f(outsidePositive ? 1.0f : -1.0f, 0.0f, 0.0f); break;
			case 'y': glNormal3f(0.0f, outsidePositive ? 1.0f : -1.0f, 0.0f); break;
			case 'z': glNormal3f(0.0f, 0.0f, outsidePositive ? 1.0f : -1.0f); break;
			default: errorMsg("Invalid dir in Wall::draw('%c')\n", dir);
		}
		Vertex *qv = quad.vertices;
		for (int i=0; i < 4; i++) {
			//if (firstTime) debugMsg("(%1.1f %1.1f %1.1f) ", qv[i].x, qv[i].y, qv[i].z);
			glTexCoord2f(qv[i].u, qv[i].v); glVertex3f(qv[i].x, qv[i].y, qv[i].z);
		}

		//if (firstTime) debugMsg("\n");
	}

	// Outside a glBegin/glEnd, draw a visual marker for the exit (or entrance) at this wall.
	void drawExit(int x, int y, int z, bool isEntrance) {
		Vertex *qv = quad.vertices;
		GLfloat cx = qv[0].x + cellSize * 0.5f,
			cy = qv[0].y + cellSize * 0.5f,
			cz = qv[0].z + cellSize * 0.5f; // center of disc
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

		// no texture
		glBindTexture(GL_TEXTURE_2D, GL_NONE);
		// color: red or green
		if (isEntrance) glColor3f(0.4f, 1.0f, 0.4f);
		else glColor3f(1.0f, 0.4f, 0.4f);

		// spin polygon (disc)
		exitRot += 1.25f;
		glRotatef(isEntrance ? exitRot : -exitRot, 0.0f, 0.0f, 1.0f);

		// gluDisk(quadric, innerRadius, outerRadius, slices, loops)
		gluDisk(quadric, cellSize * 0.4f, cellSize * 0.5f, 5, 3);

		glPopMatrix();
	}
};

/* Main model: the maze consists of both cells and walls. Each wall may be shared by two cells. */
/* C++ doesn't support dynamic multidimensional arrays very well, hence the following cruft. */
// The maze's walls
Wall (*xWalls)[hMax][dMax];    // Walls facing along X axis   [w+1][h][d]
Wall (*yWalls)[hMax+1][dMax]; // Walls facing along Y axis   [w][h+1][d]
Wall (*zWalls)[hMax][dMax+1];  // Walls facing along Z axis   [w][h][d+1]

// Pointers to the open "walls" at entrance and exit.
Wall *exitWall, *entranceWall;

class Cell {
public:
	enum CellState { uninitialized, passage, forbidden } state;
	// state = uninitialized: This cell has not been visited yet.
	// state = passage: This cell has been carved out; it is "passageway".
	// state = forbidden: true iff this cell is too close to others and so must be unoccupied.
	Cell() { state = uninitialized; }
	static inline CellState getCellState(int x, int y, int z);
};
Cell (*cells)[hMax][dMax];		// The maze's cells

Cell::CellState Cell::getCellState(int x, int y, int z) { return cells[x][y][z].state; };

class CellCoord {
public:
	int x, y, z;
	CellCoord(int a=0, int b=0, int c=0) { x = a, y = b, z = c; };
	void init(glPoint &p) {
		x = int(floor(p.x / cellSize));
		y = int(floor(p.y / cellSize));
		z = int(floor(p.z / cellSize));
	}
	boolean operator ==(CellCoord &nc) { return (x == nc.x && y == nc.y && z == nc.z); }
	bool isCellPassage() { return cells[x][y][z].state == Cell::passage; }
	void setCellState(Cell::CellState v) { cells[x][y][z].state = v; }
	inline Cell::CellState getCellState() { return cells[x][y][z].state; }
	// distance squared (euclidean)
	inline int dist2(int i, int j, int k) { return (x - i)*(x - i) + (y - j)*(y - j) + (z - k)*(z - k); }
	// manhattan distance
	inline int manhdist(CellCoord &nc) { return abs(x - nc.x) + abs(y - nc.y) + abs(z - nc.z); }
	/* Return true iff this cell is a legal place to expand into from fromCC.
		Assumes both are within bounds of maze grid, that they differ in only one axis and
		by only one unit, that fromCC is already passage, and that this cell is uninitialized.
		Checks whether this cell is too close to other passages, violating sparsity. */
	bool isCellPassable(CellCoord *fromCC) {
		/* Trivial case: if sparsity == 1, no check for closeness. */
		if (sparsity <= 1) return true;

		// get direction of movement
		int dx = x - fromCC->x, dy = y - fromCC->y, dz = z - fromCC->z;
		// compute bounding box to check: lower & upper bounds for i, j, k ####
		int i, j, k, il, iu, jl, ju, kl, ku;
		// radius squared
		int sparsity2 = sparsity * sparsity;
		il = x - sparsity + 1;
		iu = x + sparsity - 1;
		jl = y - sparsity + 1;
		ju = y + sparsity - 1;
		kl = z - sparsity + 1;
		ku = z + sparsity - 1;
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
		iu = min(iu, w-1);
		ju = min(ju, h-1);
		ku = min(ku, d-1);

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
	Wall *findWallTo(CellCoord *nc) {
		Wall *w;
		if (nc->x != x)	{
			w = &(xWalls[(nc->x > x) ? nc->x : x][y][z]);
			//debugMsg("   found xWalls[%d %d %d]\n", ((nc->x > x) ? nc->x : x), y, z);
		}
		else if (nc->y != y) {
			w = &(yWalls[x][(nc->y > y) ? nc->y : y][z]);
			//debugMsg("   found yWalls[%d %d %d]\n", x, ((nc->y > y) ? nc->y : y), z);
		}
		else {
			w = &(zWalls[x][y][(nc->z > z) ? nc->z : z]);
			//debugMsg("   found zWalls[%d %d %d]\n", x, y, ((nc->z > z) ? nc->z : z));
		}
		return w;
	}
	/* Set wall between this and nc to given state.
	 * See assumptions of findWallTo().
	 * Also assumes this is "inside" and nc is "outside", if it matters
	 (this assumption can be incorrect if sparseness <= 1). */
	void setStateOfWallTo(CellCoord *nc, Wall::WallState state) {
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
	void setStateOfWallTo(int dx, int dy, int dz, Wall::WallState state) {
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
	inline Wall *tryOpenWall(int dx, int dy, int dz) {
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
	Wall *openAWall() {
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
	void standOutside(Wall *w) {
		GLfloat cx = (x + 0.5f) * cellSize, cy = (y + 0.5f) * cellSize, cz = (z + 0.5f) * cellSize;
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
	 * See assumptions of findWallTo(). */
	Wall::WallState getStateOfWallTo(CellCoord *nc) {
		Wall *w = findWallTo(nc);
		return w->state;
	}
	/* Get wall of this cell in direction dx,dy,dz.
	 * See assumptions of findWallTo(). */
	Wall::WallState getStateOfWallTo(int dx, int dy, int dz) {
		static CellCoord nc;
		nc.x = x+dx; nc.y = y+dy; nc.z = z+dz;
		return getStateOfWallTo(&nc);
	}
	/* Get state of wall between this and nc.
	 * Checks boundaries: does not assume coords are in range. */
	Wall::WallState getStateOfWallToSafe(CellCoord *nc) {
		//debugMsg(" gSOWTS(%d, %d, %d) ", nc->x, nc->y, nc->z);
		// Is this right?
		if ((x >= w && nc->x >= w) ||
			(y >= h && nc->y >= h) ||
			(z >= d && nc->z >= d) ||
			(x < 0 && nc->x < 0) ||
			(y < 0 && nc->y < 0) ||
			(z < 0 && nc->z < 0))
		{ // debugMsg(" ob1 ");
		return Wall::OPEN; }
		if (x > w || nc->x > w ||
			y > h || nc->y > h ||
			z > d || nc->z > d ||
			x < -1 || nc->x < -1 ||
			y < -1 || nc->y < -1 ||
			z < -1 || nc->z < -1)
		{ // debugMsg(" ob2 ");
		return Wall::OPEN; }

		Wall *w = findWallTo(nc);
		//debugMsg(" wstate: %d ", w->state);
		return w->state;
	}

	// Convenience function for Wall::drawExit()
	inline void drawExit(Wall *w, bool isEntrance) {
		w->drawExit(x, y, z, isEntrance);
	}

	/* Randomly shuffle the order of CellCoords in an array.
	 * Ref: http://en.wikipedia.org/wiki/Fisher-Yates_shuffle#The_modern_algorithm */
	static void shuffleCCs(CellCoord *ccs, int n) {
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
};

CellCoord ccExit, ccEntrance;

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

// Generate a random maze with no loops, where every cell is reachable from every other.
// The entrance and exit could be placed anywhere.
// 
// This has been modified to add sparsity: passage cells cannot be
// within distance k of each other unless they are connected as directly as possible.
void generateMaze()
{
	CellCoord *queue = new CellCoord[w*h*d], *currCell, temp, neighbors[6];
	int queueSize = 0, addedNeighbors = 0, eemhdist = 0, ncmhdist = 0;

	srand((int)time(0));
	// pick a random starting cell and put it in the queue
	queue[0].x = rand() % w;
	queue[0].y = rand() % h;
	queue[0].z = rand() % d;
	queue[0].setCellState(Cell::passage);
	ccEntrance = ccExit = queue[0];
	queueSize++;

	do {
		// select a cell at random from the queue
		currCell = &(queue[rand() % queueSize]);
		//debugMsg("Picked cell %d %d %d. ", currCell->x, currCell->y, currCell->z);

		// Should this be the new entrance or exit?
		// if currCell is further from entrance than exit is, set exit to currCell
		ncmhdist = currCell->manhdist(ccEntrance);
		if (ncmhdist > eemhdist) {
			//debugMsg("Replaced exit %d,%d,%d with %d,%d,%d: %d from entrance %d,%d,%d\n",
			//	ccExit.x,ccExit.y,ccExit.z, currCell->x,currCell->y,currCell->z,
			//	ncmhdist, ccEntrance.x,ccEntrance.y,ccEntrance.z);
			ccExit = *currCell;
			eemhdist = ncmhdist;
		}
		// if currCell is further from exit than entrance is, set entrance to currCell
		else {
			ncmhdist = currCell->manhdist(ccExit);
			if (ncmhdist > eemhdist) {
				//debugMsg("Replaced entrance %d,%d,%d with %d,%d,%d: %d from exit %d,%d,%d\n",
				//	ccEntrance.x,ccEntrance.y,ccEntrance.z, currCell->x,currCell->y,currCell->z,
				//	ncmhdist, ccExit.x,ccExit.y,ccExit.z);
				ccEntrance = *currCell;
				eemhdist = ncmhdist;
			}
		}

		// enumerate neighbors:
		int nn = 0; // num of neighbors
		if (currCell->x > 0)     { neighbors[nn] = *currCell; neighbors[nn++].x--; }
		else currCell->setStateOfWallTo(-1, 0, 0, Wall::CLOSED);
		if (currCell->x < w - 1) { neighbors[nn] = *currCell; neighbors[nn++].x++; }
		else currCell->setStateOfWallTo( 1, 0, 0, Wall::CLOSED);
		if (currCell->y > 0)     { neighbors[nn] = *currCell; neighbors[nn++].y--; }
		else currCell->setStateOfWallTo(0, -1, 0, Wall::CLOSED);
		if (currCell->y < h - 1) { neighbors[nn] = *currCell; neighbors[nn++].y++; }
		else currCell->setStateOfWallTo(0,  1, 0, Wall::CLOSED);
		if (currCell->z > 0)     { neighbors[nn] = *currCell; neighbors[nn++].z--; }
		else currCell->setStateOfWallTo(0, 0, -1, Wall::CLOSED);
		if (currCell->z < d - 1) { neighbors[nn] = *currCell; neighbors[nn++].z++; }
		else currCell->setStateOfWallTo(0, 0,  1, Wall::CLOSED);
		//debugMsg("%d neighbors.\n", nn);
		// maybe shuffle order of neighbors, to avoid predictable maze shapes.
		CellCoord::shuffleCCs(neighbors, nn);

		addedNeighbors = 0;

		// for each neighbor:
		for (int i=0; i < nn; i++) {
			CellCoord *ni = &(neighbors[i]);
			Cell::CellState state = ni->getCellState();
			bool passable = false;

			switch (state) {
				case Cell::passage:
				case Cell::forbidden:
					// Close the walls between currCell and any neighbors, if the wall isn't already explicitly opened.
					// This can only happen to "passage" neighbors if sparsity < 2.
					if (currCell->getStateOfWallTo(ni) == Wall::UNINITIALIZED)
						currCell->setStateOfWallTo(ni, Wall::CLOSED);
					break;
				case Cell::uninitialized:
					// We have the potential for expansion into this neighbor.
					// Tend to not expand into too many neighbors from one cell (limit branch clustering).
					if (rand() % (addedNeighbors + 1) > branchClustering) {
						// don't expand into neighbor, but don't mark it forbidden either.
						currCell->setStateOfWallTo(ni, Wall::CLOSED);
						break;
					}

					// Evaluate whether the neighbor could become passage or not, based on proximity
					// to other passages.
					// If it can't, mark as forbidden.
					// If it can, open the walls between currCell and neighbor,
					// mark the neighbor as passage and put it into the queue.
					passable = ni->isCellPassable(currCell);
					//debugMsg("  Neighbor %d %d %d is passable? %c\n", ni->x, ni->y, ni->z, passable ? 'y' : 'n');

					if (passable) {
						currCell->setStateOfWallTo(ni, Wall::OPEN);
						ni->setCellState(Cell::passage);
						queue[queueSize++] = *ni;
						addedNeighbors++;
					} else {
						currCell->setStateOfWallTo(ni, Wall::CLOSED);
						ni->setCellState(Cell::forbidden);
					}
					break;
				default:
					fprintf(stderr, "Error: invalid cell state %d for neighbor %d %d %d.\n",
						state, ni->x, ni->y, ni->z);
					assert(state == Cell::uninitialized || state == Cell::passage || state == Cell::forbidden);
			}
		}

		// remove currCell from the queue (swap in last cell of queue)
		if (currCell - queue != queueSize - 1) {
			temp = *currCell;
			*currCell = queue[queueSize-1];
			queue[queueSize-1] = temp;
		}
		queueSize--;
		//debugMsg("qsize afterwards: %d\n", queueSize);
	} while (queueSize > 0);

	// Add an entrance and exit.
	entranceWall = ccEntrance.openAWall();
	exitWall = ccExit.openAWall();

	// Old method, works only for a filled maze (sparsity <= 1):
	//// We could place these anywhere, but putting them on opposite corners minimizes
	//// the risk of putting the entrance next to the exit.
	//xWalls[0][  0][  0].state = Wall::OPEN;
	//xWalls[w][h-1][d-1].state = Wall::OPEN;

	delete [] queue;
}

#define d(x) (0) // ((zWalls[0][0][0].state == 0) ? debugMsg(" [z0 @ %d] ", (x)) : 0)

void SetupWorld()
{
	const Wall::WallState outerWallState = Wall::UNINITIALIZED;
	Vertex *pVertex;
	int i, j, k;

	exitRot = 0.0f;

	// allocate wall arrays
	xWalls = new Wall[w+1][hMax][dMax];
	yWalls = new Wall[w][hMax+1][dMax];
	zWalls = new Wall[w][hMax][dMax+1];

	// allocate cell arrays
	cells = new Cell[w][hMax][dMax];

	// set up vertices and states of xWalls, yWalls, zWalls
	for (i=0; i <= w; i++)
		for (j=0; j <= h; j++)
			for (k=0; k <= d; k++) {
				if (i < w && j < h && k < d) cells[i][j][k].state = Cell::uninitialized;
				// debugMsg("%d %d %d ", i, j, k); d(1);
				if (j < h && k < d) {	// xWall:					
					xWalls[i][j][k].state = (i == 0 || i == w) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("x: %d ", xWalls[i][j][k].state); d(2);
					pVertex = &(xWalls[i][j][k].quad.vertices[0]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(xWalls[i][j][k].quad.vertices[1]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 0.0;
					pVertex = &(xWalls[i][j][k].quad.vertices[2]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*(j+1);
					pVertex->z = cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(xWalls[i][j][k].quad.vertices[3]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*(j+1);
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
				}
				if (i < w && k < d) {	// yWall:
					yWalls[i][j][k].state = (j == 0 || j == h) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("y: %d ", yWalls[i][j][k].state); d(3);
					pVertex = &(yWalls[i][j][k].quad.vertices[0]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(yWalls[i][j][k].quad.vertices[1]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 0.0;
					pVertex = &(yWalls[i][j][k].quad.vertices[2]);
					pVertex->x = cellSize*(i+1);
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(yWalls[i][j][k].quad.vertices[3]);
					pVertex->x = cellSize*(i+1);
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
				}
				if (i < w && j < h) {	// zWall:
					zWalls[i][j][k].state = (k == 0 || k == d) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("z: %d", zWalls[i][j][k].state); d(4);
					pVertex = &(zWalls[i][j][k].quad.vertices[0]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(zWalls[i][j][k].quad.vertices[1]);
					pVertex->x = cellSize*i;
					pVertex->y = cellSize*(j+1);
					pVertex->z = cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
					pVertex = &(zWalls[i][j][k].quad.vertices[2]);
					pVertex->x = cellSize*(i+1);
					pVertex->y = cellSize*(j+1);
					pVertex->z = cellSize*k;
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(zWalls[i][j][k].quad.vertices[3]);
					pVertex->x = cellSize*(i+1);
					pVertex->y = cellSize*j;
					pVertex->z = cellSize*k;
					pVertex->u = 1.0;
					pVertex->v = 0.0;
				}
				// debugMsg("\n");
				// debugMsg("zWalls[0][0][0].state = %d\n", zWalls[0][0][0].state);
			}
	generateMaze();
	// debugMsg("zWalls[0][0][0].state = %d\n", zWalls[0][0][0].state);

	// Now set up our max values for the camera
	Cam.m_MaxVelocity = wallMargin * 1.0f;
	Cam.m_MaxAccel = Cam.m_MaxVelocity * 0.5f;
	Cam.m_MaxPitchRate = 5.0f;
	Cam.m_MaxPitch = 89.9f;
	Cam.m_MaxHeadingRate = 5.0f;
	Cam.m_PitchDegrees = 0.0f;
	Cam.m_HeadingDegrees = 0.0f;
	Cam.m_Position.x = 0.0f * cellSize;
	Cam.m_Position.y = 1.0f * cellSize;
	Cam.m_Position.z = -15.0f * cellSize;

	memset((void *)keysDown, 0, sizeof(keysDown));
	memset((void *)keysStillDown, 0, sizeof(keysStillDown));
	return;
}

// To do: transfer those photos I took from the camera (walls, ceiling/floor)
AUX_RGBImageRec *LoadBMP(char *Filename)                // Loads A Bitmap Image
{
        FILE *File=NULL;                                // File Handle

        if (!Filename)                                  // Make Sure A Filename Was Given
        {
                return NULL;                            // If Not Return NULL
        }

        File=fopen(Filename,"r");                       // Check To See If The File Exists

        if (File)                                       // Does The File Exist?
        {
                fclose(File);                           // Close The Handle
                return auxDIBImageLoad(Filename);       // Load The Bitmap And Return A Pointer
        }
        return NULL;                                    // If Load Failed Return NULL
}

int loadTexture(int i, char *filepath) {
    AUX_RGBImageRec *TextureImage[1];               // Create Storage Space For The Texture
		// UINT textureArray[1]; // for jpeg
	int Status = FALSE;

    memset(TextureImage, 0, sizeof(void *)*1);        // Set The Pointer To NULL

	if (TextureImage[0] = LoadBMP(filepath)) {
        Status=TRUE;                            // Set The Status To TRUE

        glGenTextures(numFilters, &textures[numFilters*i]);          // Create 3 filtered textures

		// Create Nearest Filtered Texture
		debugMsg("Binding texture %s at %d\n", filepath, numFilters*i+0);
		glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 0]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

        // Create Linear Filtered Texture
		debugMsg("Binding texture %s at %d\n", filepath, numFilters*i+1);
        glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 1]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);

		// Create MipMapped Texture
		debugMsg("Binding texture %s at %d\n", filepath, numFilters*i+2);
		glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 2]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
		gluBuild2DMipmaps(GL_TEXTURE_2D, 3, TextureImage[0]->sizeX, TextureImage[0]->sizeY, GL_RGB, GL_UNSIGNED_BYTE, TextureImage[0]->data);
    }
    if (TextureImage[0])                            // If Texture Exists
    {
        if (TextureImage[0]->data)              // If Texture Image Exists
        {
                free(TextureImage[0]->data);    // Free The Texture Image Memory
        }

        free(TextureImage[0]);                  // Free The Image Structure
    }
	return Status;
}

int LoadGLTextures()                                    // Load Bitmaps And Convert To Textures
{
        int Status=FALSE;                               // Status Indicator
        // Load The Bitmap, Check For Errors, If Bitmap's Not Found Quit

		Status = loadTexture(brick, "Data/brick.bmp") && loadTexture(carpet, "Data/carpet.bmp")
			&& loadTexture(wood, "Data/wood-panel256.bmp") && loadTexture(popcorn, "Data/popcorn-ceiling.bmp");

		// JPEG_Texture(TextureArray, "data/wood-panel.jpg", 0);

		debugMsg("Texture load status: %d\n", Status);
        return Status;                                  // Return The Status
}

#ifdef USE_FONTS

GLvoid BuildFont(GLvoid)								// Build Our Bitmap Font
{
	HFONT	font;										// Windows Font ID
	HFONT	oldfont;									// Used For Good House Keeping

	base = glGenLists(96);								// Storage For 96 Characters

	font = CreateFont(	-24,							// Height Of Font
						0,								// Width Of Font
						0,								// Angle Of Escapement
						0,								// Orientation Angle
						FW_BOLD,						// Font Weight
						FALSE,							// Italic
						FALSE,							// Underline
						FALSE,							// Strikeout
						ANSI_CHARSET,					// Character Set Identifier
						OUT_TT_PRECIS,					// Output Precision
						CLIP_DEFAULT_PRECIS,			// Clipping Precision
						ANTIALIASED_QUALITY,			// Output Quality
						FF_DONTCARE|DEFAULT_PITCH,		// Family And Pitch
						"Courier New");					// Font Name

	oldfont = (HFONT)SelectObject(hDC, font);           // Selects The Font We Want
	wglUseFontBitmaps(hDC, 32, 96, base);				// Builds 96 Characters Starting At Character 32
	SelectObject(hDC, oldfont);							// Selects The Font We Want
	DeleteObject(font);									// Delete The Font
}

GLvoid KillFont(GLvoid)									// Delete The Font List
{
	glDeleteLists(base, 96);							// Delete All 96 Characters
}
#endif // USE_FONTS


GLvoid glPrint(const char *fmt, ...)					// Custom GL "Print" Routine
{
#ifdef USE_FONTS
	char		text[256];								// Holds Our String
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)									// If There's No Text
		return;											// Do Nothing

	va_start(ap, fmt);									// Parses The String For Variables
	    vsprintf(text, fmt, ap);						// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	glPushMatrix();
	glTranslatef(1.0f, 0.5f, 3.05f);	// position text
	glColor3f(1.0f, 1.0f, 1.0f);
	// Position The Text On The Screen
	glRasterPos2f(-0.0f, 0.35f); // Doesn't seem to do anything

	glPushAttrib(GL_LIST_BIT);							// Pushes The Display List Bits
	glListBase(base - 32);								// Sets The Base Character to 32
	glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);	// Draws The Display List Text
	glPopAttrib();										// Pops The Display List Bits
	glPopMatrix();
#endif // USE_FONTS
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, zNear, zFar);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	if (!LoadGLTextures())								// Jump To Texture Loading Routine
	{
		return FALSE;									// If Texture Didn't Load Return FALSE
	}

	glEnable(GL_TEXTURE_2D);							// Enable Texture Mapping
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);					// Set The Blending Function For Translucency
	glClearColor(0.85f, 0.9f, 1.0f, 0.0f);				// Set the background color (sky blue)
	glClearDepth(1.0);									// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LESS);								// The Type Of Depth Test To Do
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glShadeModel(GL_SMOOTH);							// Enables Smooth Color Shading
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	quadric = gluNewQuadric();			// create a quadric object for cylinders, discs, etc.
	gluQuadricNormals(quadric, GLU_SMOOTH);	// Create Smooth Normals ( NEW )
	gluQuadricTexture(quadric, GL_TRUE);		// Create Texture Coords ( NEW )

#ifdef USE_FONTS
	BuildFont();						// Build The Font
#endif

	SetupWorld();

	return TRUE;										// Initialization Went OK
}

int DrawGLScene(GLvoid)									// Here's Where We Do All The Drawing
{
	if (blend) {			// do polygon anti-aliasing, a la http://glprogramming.com/red/chapter06.html#name2
		glClear (GL_COLOR_BUFFER_BIT);
	} else {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	}

	glLoadIdentity();									// Reset The View matrix

	Cam.SetPerspective();

	// apply friction:
	Cam.m_ForwardVelocity *= 0.9f;
	Cam.m_SidewaysVelocity *= 0.9f;

	// reset white color (default)
	glColor3f(1.0f, 1.0f, 1.0f);

	//if (firstTime) debugMsg("beginning walls loop\n");

	// debugMsg("zWalls[0][0][0].state = %d\n", zWalls[0][0][0].state);

	// Brick texture: xWalls
	glBindTexture(GL_TEXTURE_2D, textures[brick*numFilters+filter]);

#ifdef DEBUGGING
	//// cylinders for z-axis identification
	//gluCylinder(quadric, 0.2f, 0.2f, 5.0f, 10, 10);
#endif

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i <= w; i++)
		for (int j=0; j < h; j++)
			for (int k=0; k < d; k++) {
				if (xWalls[i][j][k].state == Wall::CLOSED) {
					//if (firstTime) {
					//	debugMsg("drawing x wall %d,%d,%d: ", i, j, k); 
					//}
					xWalls[i][j][k].draw('x');	// draw xWall
				}
			}
	glEnd();

	// Wood panel texture: zWalls
	glBindTexture(GL_TEXTURE_2D, textures[wood*numFilters+filter]);

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < w; i++)
		for (int j=0; j < h; j++)
			for (int k=0; k <= d; k++) {

				if (zWalls[i][j][k].state == Wall::CLOSED) {
					//if (firstTime) {
					//	debugMsg("drawing z wall %d,%d,%d: ", i, j, k); 
					//}
					zWalls[i][j][k].draw('z'); // draw zWall
				}
			}
	glEnd();

	// floor texture
	glBindTexture(GL_TEXTURE_2D, textures[carpet*numFilters+filter]);
	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < w; i++)
		for (int j=0; j <= h; j++)
			for (int k=0; k < d; k++) {
				if (yWalls[i][j][k].state == Wall::CLOSED
					&& !yWalls[i][j][k].outsidePositive) {
					//if (firstTime) {
					//	debugMsg("drawing y wall carpet %d,%d,%d\n", i, j, k);
					//}
					yWalls[i][j][k].draw('y'); // draw yWall
				}
			}

	glEnd();

	// popcorn ceiling texture
	glBindTexture(GL_TEXTURE_2D, textures[popcorn*numFilters+filter]);
	// glColor3f(1.0f, 0.5f, 0.5f); // temp for debugging
	//if (firstTime) {
	//	debugMsg("Binding popcorn texture at %d\n", popcorn*numFilters+filter);
	//}

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < w; i++)
		for (int j=0; j <= h; j++)
			for (int k=0; k < d; k++) {
				if (yWalls[i][j][k].state == Wall::CLOSED
					&& yWalls[i][j][k].outsidePositive) {
					//if (firstTime) {
					//	debugMsg("drawing y wall popcorn %d,%d,%d\n", i, j, k);
					//}
					yWalls[i][j][k].draw('y'); // draw yWall
				}
			}

	glEnd();

	// display entrance/exit (may mess up rot/transf matrix)
	ccEntrance.drawExit(entranceWall, true);
	ccExit.drawExit(exitWall, false);

	firstTime = false;									// debugging
	return TRUE;										// Everything Went OK
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}

#ifdef USE_FONTS
	KillFont();
#endif
}

/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","NeHe GL",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);		// Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{
	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keysDown[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keysDown[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			return 0;								// Jump Back
		}
		case WM_LBUTTONDOWN:				// Did We Receive A Left Mouse Click?
		{
			mouseGrab = !mouseGrab; // toggle mouse steering
			return 0;
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

void freeResources() {
	delete [] cells;
	delete [] xWalls;
	delete [] yWalls;
	delete [] zWalls;
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	//// Ask The User Which Screen Mode They Prefer
	//if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	//{
		fullscreen=FALSE;							// Windowed Mode
	//}

	// Create Our OpenGL Window
	if (!CreateGLWindow(title, xRes, yRes, 16, fullscreen))
	{
		return 0;									// Quit If Window Was Not Created
	}

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || keysDown[VK_ESCAPE])	// Active?  Was There A Quit Received?
			{
				done=TRUE;							// ESC or DrawGLScene Signalled A Quit
			}
			else									// Not Time To Quit, Update Screen
			{
				SwapBuffers(hDC);					// Swap Buffers (Double Buffering)
				if (CheckKeys()) done = true;
				else CheckMouse();
			}
		}
	}

	// Shutdown
	KillGLWindow();										// Kill The Window
	freeResources();
	return ((int)msg.wParam);								// Exit The Program
}


/* Check and process keyboard input. 
 * Return true iff quit requested. */
bool CheckKeys(void) {
	// Process arrow (turn) keys.
	if(keysDown[VK_UP]) Cam.ChangePitch(keyTurnRate);
	if(keysDown[VK_DOWN]) Cam.ChangePitch(-keyTurnRate);
	if(keysDown[VK_LEFT]) Cam.ChangeHeading(-keyTurnRate);	
	if(keysDown[VK_RIGHT]) Cam.ChangeHeading(keyTurnRate);
	
	// For now, no gliding.
	// Cam.m_ForwardVelocity = 0.0f;
	// Cam.m_SidewaysVelocity = 0.0f;

	// Process WSAD (movement) keys. (Allow for dvorak layout too.)
	if(keysDown['W'] || keysDown[VK_OEM_COMMA]) Cam.AccelForward(keyAccelRate);	
	if(keysDown['S'] || keysDown['O']) Cam.AccelForward(-keyAccelRate);
	if(keysDown['A']) Cam.AccelSideways(-keyAccelRate);
	if(keysDown['D'] || keysDown['E']) Cam.AccelSideways(keyAccelRate);

	// reset position / heading / pitch to the beginning of the maze.
	if (keysDown[VK_HOME] && !keysStillDown[VK_HOME]) {
		keysStillDown[VK_HOME]=TRUE;
		ccEntrance.standOutside(entranceWall);
	}
	else if (!keysDown[VK_HOME])
	{
		keysStillDown[VK_HOME]=FALSE;
	}

	// snap to grid
	if (keysDown[VK_SPACE] && !keysStillDown[VK_SPACE]) {
		keysStillDown[VK_SPACE]=TRUE;
		Cam.SnapToGrid();
	}
	else if (!keysDown[VK_SPACE])
	{
		keysStillDown[VK_SPACE]=FALSE;
	}

	if (keysDown['B'] && !keysStillDown['B'])
	{
		keysStillDown['B']=TRUE;
		blend=!blend;
		if (blend)
		{
			glEnable(GL_BLEND);
			glEnable (GL_POLYGON_SMOOTH);
			// glDisable(GL_DEPTH_TEST);

			glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			// was (GL_SRC_ALPHA_SATURATE, GL_ONE); see http://www.opengl.org/documentation/specs/man_pages/hardcopy/GL/html/gl/blendfunc.html
			/* "Blend function (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) is
				also useful for rendering antialiased points and lines in
				arbitrary order.
				Polygon antialiasing is optimized using blend function
				(GL_SRC_ALPHA_SATURATE, GL_ONE) with polygons sorted from
				nearest to farthest." */
		}
		else
		{
			glDisable(GL_BLEND);
			glDisable(GL_POLYGON_SMOOTH);
			glEnable(GL_DEPTH_TEST);
		}
	}
	else if (!keysDown['B'])
	{
		keysStillDown['B']=FALSE;
	}

	if (keysDown['P'] && !keysStillDown['P'])
	{
		// toggle autopilot
		keysStillDown['P']=TRUE;
		autopilot = !autopilot;
	}
	else if (!keysDown['P'])
	{
		keysStillDown['P']=FALSE;
	}

	if (keysDown['M'] && !keysStillDown['M'])
	{
		// toggle mouseGrab
		keysStillDown['M']=TRUE;
		mouseGrab = !mouseGrab;
	}
	else if (!keysDown['M'])
	{
		keysStillDown['M']=FALSE;
	}

	if (keysDown['C'] && !keysStillDown['C'])
	{
		// toggle collision checks
		keysStillDown['C']=TRUE;
		checkCollisions = !checkCollisions;
		// should we give some visual feedback?
	}
	else if (!keysDown['C'])
	{
		keysStillDown['C']=FALSE;
	}

	if (keysDown['F'] && !keysStillDown['F'])
	{
		keysStillDown['F']=TRUE;
		filter = (filter + 1) % numFilters;
	}
	else if (!keysDown['F'])
	{
		keysStillDown['F']=FALSE;
	}

	//if (keysDown['N'] && !keysStillDown['N'])
	//{
	//	keysStillDown['N']=TRUE;
	//	toggleAAPolys();
	//}
	//else if (!keysDown['N'])
	//{
	//	keysStillDown['N']=FALSE;
	//}

	/* Old code from lesson10:
	if (keysDown[VK_UP])
	{

		xpos -= (float)sin(yheading*piover180) * 0.05f;
		zpos -= (float)cos(yheading*piover180) * 0.05f;
	}

	if (keysDown[VK_DOWN])
	{
		xpos += (float)sin(yheading*piover180) * 0.05f;
		zpos += (float)cos(yheading*piover180) * 0.05f;
	}

	if (keysDown[VK_RIGHT])
	{
		yheading -= 1.0f;
	}

	if (keysDown[VK_LEFT])
	{
		yheading += 1.0f;	
	}

	if (keysDown[VK_PRIOR])
	{
		xheading-= 1.0f;
	}

	if (keysDown[VK_NEXT])
	{
		xheading+= 1.0f;
	}
	*/

	if (keysDown[VK_F1])						// Is F1 Being Pressed?
	{
		keysDown[VK_F1]=FALSE;					// If So Make Key FALSE
		KillGLWindow();						// Kill Our Current Window
		fullscreen = !fullscreen;				// Toggle Fullscreen / Windowed Mode
		// Recreate Our OpenGL Window
		if (!CreateGLWindow(title, xRes, yRes, 16, fullscreen))
		{
			return true;						// Quit If Window Was Not Created
		}
	}
	return false;
}

void CheckMouse(void)
{
	GLfloat DeltaMouse;
	POINT pt;

	if (!mouseGrab) return;

	GetCursorPos(&pt);
	
	CenterX = xRes / 2; // don't know how to get actual res of window
	CenterY = yRes / 2;

	MouseX = pt.x;
	MouseY = pt.y;

	if(MouseX < CenterX)
	{
		DeltaMouse = GLfloat(CenterX - MouseX);

		Cam.ChangeHeading(-0.2f * DeltaMouse);
		
	}
	else if(MouseX > CenterX)
	{
		DeltaMouse = GLfloat(MouseX - CenterX);

		Cam.ChangeHeading(0.2f * DeltaMouse);
	}

	if(MouseY < CenterY)
	{
		DeltaMouse = GLfloat(CenterY - MouseY);

		Cam.ChangePitch(-0.2f * DeltaMouse);
	}
	else if(MouseY > CenterY)
	{
		DeltaMouse = GLfloat(MouseY - CenterY);

		Cam.ChangePitch(0.2f * DeltaMouse);
	}

	MouseX = CenterX;
	MouseY = CenterY;

	SetCursorPos(CenterX, CenterY);
}

/* Return true iff moving from point p along vector v would collide with a wall.
 * Modify p to be p + v, adjusted to remove collision; thus we "slide" along a wall.
 * ##To do: maybe "friction" should then slow down the rest of v.
 * In this implementation, we ignore direction/magnitude of v itself and simply check whether
 * (p+v) is too close to (within wallMargin of) a wall.
 * Works well if maximum speed |v| < wallMargin*2.
 * Certainly |v| must be less than cellSize, i.e. we won't cross multiple cells at once.
 * "A wall" means a square unit of the plane where the wall state is CLOSED.
 * "A potential wall" means a square unit of the plane, regardless of its wall state.
 *
 * To debug: it is possible to go through walls if you head upward diagonally toward
 * just above a bottom corner of the maze.
 */
bool collide(glPoint &p, glVector &v)
{
	if (!checkCollisions) {
		p += v;
		return false;
	}
	bool result = false; // needed?
	CellCoord qcc, ncc; // cell coords of cell that p+v is in, and of neigbor cell
	glPoint q;
	// Note that for some reason the glCamera code reverses the z axis of p and v. So reverse it back.
	p.z = -p.z;
	v.k = -v.k;

	q = p;
	q += v;
	
	qcc.init(q);
	ncc = qcc;

	// debugMsg("In collide(<%.2f %.2f %.2f>... ", q.x, q.y, q.z);

	// For each of the six possible walls:
	// - test whether possible wall is within maze
	//   and whether possible wall is a wall
	// - test if q is within wallMargin of wall
	ncc.x = qcc.x - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.x - (qcc.x * cellSize) < wallMargin)) {
		q.x = qcc.x * cellSize + wallMargin;
		result = true;
	} else {
		ncc.x = qcc.x + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.x * cellSize) - q.x < wallMargin) {
			q.x = ncc.x * cellSize - wallMargin;
			result = true;
		}
	}
	ncc.x = qcc.x; // restore it

	ncc.y = qcc.y - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.y - (qcc.y * cellSize) < wallMargin)) {
		q.y = qcc.y * cellSize + wallMargin;
		result = true;
	} else {	
		ncc.y = qcc.y + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.y * cellSize) - q.y < wallMargin) {
			q.y = ncc.y * cellSize - wallMargin;
			result = true;
		}
	}
	ncc.y = qcc.y; // restore it

	ncc.z = qcc.z - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.z - (qcc.z * cellSize) < wallMargin)) {
		q.z = qcc.z * cellSize + wallMargin;
		result = true;
	} else {	
		ncc.z = qcc.z + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.z * cellSize) - q.z < wallMargin) {
			q.z = ncc.z * cellSize - wallMargin;
			result = true;
		}
	}

	// if (!result) debugMsg(" no collision.\n");
	// Adjust position according to velocity, modified by collision.
	p.x = q.x;
	p.y = q.y;
	p.z = -q.z; // flip z back
	v.k = -v.k; // flip z back
	return result;
}
