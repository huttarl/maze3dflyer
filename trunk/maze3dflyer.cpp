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
 *
 *		See also http://www.rednebula.com/index.php?page=3dmaze for another 3D maze generation/
 *		display/navigation program (this one is not related to it).
 */

/*
 * Terminology note:
 * I have sometimes used the following terms interchangeably: visited (of a cell), passage, open, occupied, carved
 * But after algorithm changes, a cell having been "visited" (by the algorithm) no longer implies that it is "passage",
 * as cells that are not part of the navigable maze can now exist.
 */

#include <stdlib.h> // In Windows, stdlib.h must come before glut.h
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <cstdlib> //FIXME: is this portable?
// #include <time.h>
#include <ctime> //FIXME: is this portable?
#include <new> //FIXME: is this portable?
#include <assert.h>

#include <gl/gl.h>			// OpenGL header files
#include <gl/glu.h>			// 
#include <gl/glut.h>		// (glut.h actually includes gl and glu.h, so we're redundant)

// #define USE_JPG // Use BMP format for now instead of JPG. Currently JPG loading has stripey problems.
#define DEBUGGING 1

#include "maze3dflyer.h"
#include "Wall.h"
#include "Maze3D.h"
#include "CellCoord.h"
#include "Autopilot.h"
#include "HighScoreList.h"
#include "glCamera.h"
#ifdef USE_JPG
#   include "jpeg.h"
#endif

void setAutopilotMode(bool newAPM);

// Maze::collide?
/* Return true iff moving from point p along vector v would collide with a wall. */
bool collide(glPoint &p, glVector &v);

void CheckMouse(void);
bool CheckKeys(void);


glCamera Cam;				// Our Camera for moving around and setting prespective
							// on things.
Autopilot *ap;				// autopilot instance

HDC			hDC=NULL;		// Private GDI Device Context
HGLRC		hRC=NULL;		// Permanent Rendering Context
HWND		hWnd=NULL;		// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

GLuint	helpDLInner, helpDLOuter, fpsDLInner, fpsDLOuter, solvDLInner, solvDLOuter, facadeDL; // display list ID's

const float piover180 = 0.0174532925f;
int xRes = 1024;
int yRes = 768;
char title[] = "3D Maze Flyer";

bool	keysDown[256];		// keys for which we have received down events (and no release)
bool	keysStillDown[256];	// keys for which we have already processed inital down events (and no release)
							// keysStillDown[] is the equivalent of what used to be mp, bp, etc.
bool	active=TRUE;		// Window Active Flag Set To TRUE By Default
bool	fullscreen=FALSE;	// Fullscreen Flag Set To Fullscreen Mode By Default
bool	blend=FALSE;		// Blending ON/OFF
bool	autopilotMode=TRUE;		// autopilotMode on?
bool	mouseGrab=TRUE;		// mouse centering is on?
bool	showFPS=FALSE;		// whether to display frames-per-second stat
bool	showHelp=TRUE;		// show help text

// these should probably move to glCamera or a subclass thereof.
float	keyTurnRate = 2.0f; // how fast to turn in response to keys
float	keyAccelRate = 0.1f; // how fast to accelerate in response to keys
float	keyMoveRate = 0.1f;  // how fast to move in response to keys

// Used for mouse steering
UINT	MouseX, MouseY;		// Coordinates for the mouse
UINT	CenterX, CenterY;	// Coordinates for the center of the screen.

Maze3D maze;
HighScoreList highScoreList;

// distance from eye to near clipping plane. Should be about wallMargin/2.
const float zNear = (Maze3D::wallMargin * 0.6f);
// distance from eye to far clipping plane.
const float zFar = (Maze3D::cellSize * 100.0f);


GLfloat xheading = 0.0f, yheading = 0.0f;
GLfloat xpos = 0.5f, ypos = 0.5f, zpos = 10.0f;
GLUquadricObj *quadric;	

const int numFilters = 3;		// How many filters for each texture
GLuint filter = 2;				// Which filter to use
const int numTextures = (roof - ground + 1);		// Max # textures (not counting filtering)
GLuint	textures[numFilters * numTextures];		// Storage for texture indexes, with 3 filters each.

//TODO: read this from a text file at runtime?
static char helpText[] = "Controls:\n\
\n\
Esc: exit\n\
?: toggle display of help text\n\
\n\
WASD: move\n\
Arrow keys: turn\n\
Mouse: steer (if mouse grab is on)\n\
Home/End: jump to maze entrance/exit\n\
Enter: toggle autopilot (not yet implemented)\n\
P: show path from entrance to exit (not yet implemented)\n\
Space: snap camera position/orientation to grid\n\
\n\
M or left-click: toggle mouse grab\n\
T: toggle frames-per-second display\n\
F: cycle texture filter mode\n\
C: toggle collision checking (allow passing through walls or not)\n\
F1: toggle full-screen mode";

const int howLongShowSolved = 5; // for how many do we display "SOLVED"?


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


LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

// Generate a random maze with no loops, where every cell is reachable from every other.
// The entrance and exit could be placed anywhere.
// 
// This has been modified to add sparsity: passage cells cannot be
// within distance k of each other unless they are connected as directly as possible.
void generateMaze()
{
	CellCoord *queue = new CellCoord[maze.w * maze.h * maze.d], *currCell, temp, neighbors[6];
	int queueSize = 0, addedNeighbors = 0, eemhdist = 0, ncmhdist = 0;

	srand((int)time(0));
	// pick a random starting cell and put it in the queue
	queue[0].x = rand() % maze.w;
	queue[0].y = rand() % maze.h;
	queue[0].z = rand() % maze.d;
	queue[0].setCellState(Cell::passage);
	maze.ccEntrance = maze.ccExit = queue[0];
	queueSize++;

	do {
		// select a cell at random from the queue
		currCell = &(queue[rand() % queueSize]);
		//debugMsg("Picked cell %d %d %d. ", currCell->x, currCell->y, currCell->z);

		// Should this be the new entrance or exit?
		// if currCell is further from entrance than exit is, set exit to currCell
		ncmhdist = currCell->manhdist(maze.ccEntrance);
		if (ncmhdist > eemhdist) {
			//debugMsg("Replaced exit %d,%d,%d with %d,%d,%d: %d from entrance %d,%d,%d\n",
			//	ccExit.x,ccExit.y,ccExit.z, currCell->x,currCell->y,currCell->z,
			//	ncmhdist, ccEntrance.x,ccEntrance.y,ccEntrance.z);
			maze.ccExit = *currCell;
			eemhdist = ncmhdist;
		}
		// if currCell is further from exit than entrance is, set entrance to currCell
		else {
			ncmhdist = currCell->manhdist(maze.ccExit);
			if (ncmhdist > eemhdist) {
				//debugMsg("Replaced entrance %d,%d,%d with %d,%d,%d: %d from exit %d,%d,%d\n",
				//	ccEntrance.x,ccEntrance.y,ccEntrance.z, currCell->x,currCell->y,currCell->z,
				//	ncmhdist, ccExit.x,ccExit.y,ccExit.z);
				maze.ccEntrance = *currCell;
				eemhdist = ncmhdist;
			}
		}

		// enumerate neighbors:
		int nn = 0; // num of neighbors
		if (currCell->x > 0)     { neighbors[nn] = *currCell; neighbors[nn++].x--; }
		else currCell->setStateOfWallTo(-1, 0, 0, Wall::CLOSED);
		if (currCell->x < maze.w - 1) { neighbors[nn] = *currCell; neighbors[nn++].x++; }
		else currCell->setStateOfWallTo( 1, 0, 0, Wall::CLOSED);
		if (currCell->y > 0)     { neighbors[nn] = *currCell; neighbors[nn++].y--; }
		else currCell->setStateOfWallTo(0, -1, 0, Wall::CLOSED);
		if (currCell->y < maze.h - 1) { neighbors[nn] = *currCell; neighbors[nn++].y++; }
		else currCell->setStateOfWallTo(0,  1, 0, Wall::CLOSED);
		if (currCell->z > 0)     { neighbors[nn] = *currCell; neighbors[nn++].z--; }
		else currCell->setStateOfWallTo(0, 0, -1, Wall::CLOSED);
		if (currCell->z < maze.d - 1) { neighbors[nn] = *currCell; neighbors[nn++].z++; }
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
					if (rand() % (addedNeighbors + 1) > maze.branchClustering) {
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
	maze.entranceWall = maze.ccEntrance.openAWall();
	maze.exitWall = maze.ccExit.openAWall();

	// Old method, works only for a filled maze (sparsity <= 1):
	//// We could place these anywhere, but putting them on opposite corners minimizes
	//// the risk of putting the entrance next to the exit.
	//xWalls[0][  0][  0].state = Wall::OPEN;
	//xWalls[w][h-1][d-1].state = Wall::OPEN;

	delete [] queue;
}

#define d(x) (0) // ((maze.zWalls[0][0][0].state == 0) ? debugMsg(" [z0 @ %d] ", (x)) : 0)

void SetupWorld()
{
	const Wall::WallState outerWallState = Wall::UNINITIALIZED;
	Vertex *pVertex;
	int i, j, k;

	maze.exitRot = 0.0f;

	// allocate wall arrays
	maze.xWalls = new Wall[maze.w+1][Maze3D::hMax][Maze3D::dMax];
	maze.yWalls = new Wall[maze.w][Maze3D::hMax+1][Maze3D::dMax];
	maze.zWalls = new Wall[maze.w][Maze3D::hMax][Maze3D::dMax+1];

	// allocate cell arrays
	maze.cells = new Cell[maze.w][Maze3D::hMax][Maze3D::dMax];

	// set up vertices and states of xWalls, yWalls, zWalls
	for (i=0; i <= maze.w; i++)
		for (j=0; j <= maze.h; j++)
			for (k=0; k <= maze.d; k++) {
				if (i < maze.w && j < maze.h && k < maze.d) maze.cells[i][j][k].state = Cell::uninitialized;
				// debugMsg("%d %d %d ", i, j, k); d(1);
				if (j < maze.h && k < maze.d) {	// xWall:					
					maze.xWalls[i][j][k].state = (i == 0 || i == maze.w) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("x: %d ", maze.xWalls[i][j][k].state); d(2);
					pVertex = &(maze.xWalls[i][j][k].quad.vertices[0]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(maze.xWalls[i][j][k].quad.vertices[1]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 0.0;
					pVertex = &(maze.xWalls[i][j][k].quad.vertices[2]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*(j+1);
					pVertex->z = maze.cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(maze.xWalls[i][j][k].quad.vertices[3]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*(j+1);
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
				}
				if (i < maze.w && k < maze.d) {	// yWall:
					maze.yWalls[i][j][k].state = (j == 0 || j == maze.h) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("y: %d ", maze.yWalls[i][j][k].state); d(3);
					pVertex = &(maze.yWalls[i][j][k].quad.vertices[0]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(maze.yWalls[i][j][k].quad.vertices[1]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 0.0;
					pVertex = &(maze.yWalls[i][j][k].quad.vertices[2]);
					pVertex->x = maze.cellSize*(i+1);
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*(k+1);
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(maze.yWalls[i][j][k].quad.vertices[3]);
					pVertex->x = maze.cellSize*(i+1);
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
				}
				if (i < maze.w && j < maze.h) {	// zWall:
					maze.zWalls[i][j][k].state = (k == 0 || k == maze.d) ? outerWallState : Wall::UNINITIALIZED;
					// debugMsg("z: %d", maze.zWalls[i][j][k].state); d(4);
					pVertex = &(maze.zWalls[i][j][k].quad.vertices[0]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 0.0;
					pVertex = &(maze.zWalls[i][j][k].quad.vertices[1]);
					pVertex->x = maze.cellSize*i;
					pVertex->y = maze.cellSize*(j+1);
					pVertex->z = maze.cellSize*k;
					pVertex->u = 0.0;
					pVertex->v = 1.0;
					pVertex = &(maze.zWalls[i][j][k].quad.vertices[2]);
					pVertex->x = maze.cellSize*(i+1);
					pVertex->y = maze.cellSize*(j+1);
					pVertex->z = maze.cellSize*k;
					pVertex->u = 1.0;
					pVertex->v = 1.0;
					pVertex = &(maze.zWalls[i][j][k].quad.vertices[3]);
					pVertex->x = maze.cellSize*(i+1);
					pVertex->y = maze.cellSize*j;
					pVertex->z = maze.cellSize*k;
					pVertex->u = 1.0;
					pVertex->v = 0.0;
				}
				// debugMsg("\n");
				// debugMsg("zWalls[0][0][0].state = %d\n", maze.zWalls[0][0][0].state);
			}
	generateMaze();
	// debugMsg("zWalls[0][0][0].state = %d\n", maze.zWalls[0][0][0].state);

	// Now set up our max values for the camera
	Cam.m_MaxVelocity = maze.wallMargin * 1.0f;
	Cam.m_MaxAccel = Cam.m_MaxVelocity * 0.5f;
	Cam.m_MaxPitchRate = 5.0f;
	Cam.m_MaxPitch = 89.9f;
	Cam.m_MaxHeadingRate = 5.0f;
	//Cam.m_PitchDegrees = 0.0f;
	//Cam.m_HeadingDegrees = 0.0f;
	Cam.m_ForwardVelocity = 0.0f;
	Cam.m_SidewaysVelocity = 0.0f;
	//Cam.m_Position.x = 0.0f * maze.cellSize;
	//Cam.m_Position.y = 1.0f * maze.cellSize;
	//Cam.m_Position.z = -15.0f * maze.cellSize;
	maze.ccEntrance.standOutside(maze.entranceWall);

	ap = new Autopilot();
	ap->init(maze, Cam);

	memset((void *)keysDown, 0, sizeof(keysDown));
	memset((void *)keysStillDown, 0, sizeof(keysStillDown));
	return;
}

// To do: transfer those photos I took from the camera (walls, roof, ground/floor)
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

// Load an image from a file, and create textures from it using multiple filters.
// Return True if load succeeded.
bool loadTexture(int i, char *filepath) {
   bool status = TRUE;
   if(!filepath) return FALSE;

#ifdef USE_JPG
   tImageJPG *pBitMap = Load_JPEG(filepath);
#   define bitmap pBitMap
#else
   AUX_RGBImageRec *TextureImage[1];               // Create Storage Space For The Texture
   TextureImage[0] = LoadBMP(filepath);
#   define bitmap (TextureImage[0])
#endif

   if(!bitmap || !bitmap->data) return FALSE;

   glGenTextures(numFilters, &textures[numFilters*i]);          // Create 3 filtered textures

   // Create Nearest Filtered Texture
   //debugMsg("Binding texture %s at %d\n", filepath, numFilters*i + 0);
   glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 0]);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, 3, bitmap->sizeX, bitmap->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->data);

   // Create Linear Filtered Texture
   //debugMsg("Binding texture %s at %d\n", filepath, numFilters*i + 1);
   glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 1]);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
   glTexImage2D(GL_TEXTURE_2D, 0, 3, bitmap->sizeX, bitmap->sizeY, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap->data);

   // Create MipMapped Texture
   //debugMsg("Binding texture %s at %d\n", filepath, numFilters*i + 2);
   glBindTexture(GL_TEXTURE_2D, textures[numFilters*i + 2]);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
   gluBuild2DMipmaps(GL_TEXTURE_2D, 3, bitmap->sizeX, bitmap->sizeY, GL_RGB, GL_UNSIGNED_BYTE, bitmap->data);

   free(bitmap->data);			
   free(bitmap);
   return status;
}

bool LoadGLTextures()                                    // Load images and convert to textures
{
   bool status=FALSE;                               // status Indicator
   // load the image, check for errors; if it's not found, quit.

#ifdef USE_JPG
   status = loadTexture(wall1, "Data/brickWall_tileable.jpg") && loadTexture(ground, "Data/carpet-6716-2x2mir.jpg")
      && loadTexture(wall2, "Data/rocky.jpg") && loadTexture(roof, "Data/roof1.jpg") && loadTexture(portal, "Data/wood-planks-4227.jpg");
#else
   status = loadTexture(wall1, "Data/brickWall_tileable.bmp") && loadTexture(ground, "Data/carpet-6716-2x2mir.bmp")
      && loadTexture(wall2, "Data/rocky.bmp") && loadTexture(roof, "Data/roof1.bmp") && loadTexture(portal, "Data/wood-planks-4227.bmp");
#endif
   //debugMsg("Texture load status: %d\n", status);
   return status;                                  // Return The Status
}

void renderBitmapLines(float x, float y, void *font, float lineSpacing, char *str)
{
	while (*str) {
		// position to beginning of line
		glRasterPos2f(x, y);
		while (*str && *str != '\n')
			glutBitmapCharacter(font, *str++);
		if (*str == '\n') {
			y += lineSpacing;
			str++;
		}
	}
}

// Create display lists to show text on HUD.
// Changes current color.
// Creates a DL at DLOuter, and an inner DL at DLOuter + 1. 
// If those DLs exist already, they will be replaced.
GLvoid createTextDLs(GLuint DLOuter, const char *fmt, ...)
{
	GLuint DLInner = DLOuter + 1;						// nested display list
	char		text[1024];								// Holds Our String
	float x, y;
	va_list		ap;										// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
		return;											// Do Nothing

	if (strlen(fmt) > sizeof(text)) {					// obvious buffer overrun
		errorMsg("Error: oversized fmt string exceeds createTextDLs buffer[%d]: '%s'\n",
			sizeof(text), fmt);
		return;
	}

	va_start(ap, fmt);									// Parses The String For Variables
	vsprintf(text, fmt, ap);							// And Converts Symbols To Actual Numbers
	va_end(ap);											// Results Are Stored In Text

	if (DLOuter == helpDLOuter)
	    x = 5, y = 30;
	else if (DLOuter == fpsDLOuter)
	    x = xRes - 100, y = yRes - 24;
               //TODO ###: could count lines and columns in text and adjust y to yRes - 24 * lines and x to xRes - 10 * columns.
        else
           x = 5, y = yRes - 24;

	glNewList(DLInner, GL_COMPILE);
	renderBitmapLines(x, y, GLUT_BITMAP_HELVETICA_18, 24, text);
	glEndList(); // DLInner

	glNewList(DLOuter, GL_COMPILE);
	// use HL orange. :-)
	glColor3f(1.0f, 0.9f, 0.7f);
	glCallList(DLInner);
	// The following takes a long time when displaying help, but it makes
	// the text much more legible; and who cares about frame rate while
	// we're displaying help?
	glColor3f(0, 0, 0); // black shadow
	glTranslatef(1,1,0);  glCallList(DLInner);
	glTranslatef(-2,0,0); glCallList(DLInner);
	glTranslatef(0,-2,0); glCallList(DLInner);
	glTranslatef(2,0,0);  glCallList(DLInner);
	glEndList(); // DLOuter
}

// Create display lists to draw exit/entrance facade.
// If this DL exists already, it will be replaced.
GLvoid createFacadeDL(GLuint facadeDL)
{
   glNewList(facadeDL, GL_COMPILE);

   // use portal texture
   glBindTexture(GL_TEXTURE_2D, textures[portal * numFilters + filter]);

   glBegin(GL_QUADS);

   // We're drawing it here with the outside facing down.
   // So if it needs to face opposite, we need to rotate 180 degrees when drawing.

   GLfloat cellRad = Maze3D::cellSize / 2.0;
   GLfloat holeRad = Maze3D::exitHoleRadius;
   GLfloat texEdge1 = 0.5 - (holeRad / Maze3D::cellSize), texEdge2 = 0.5 + (holeRad / Maze3D::cellSize);
   GLfloat th = Maze3D::exitThickness;
   GLfloat ep = 0.0001; // epsilon
   GLuint i;

   // loop through two instances of this surface
   for (i = 0; i < 2; i++) {
      GLfloat z = (i == 0) ? 0.0 : th;
      if (i == 0)
         glNormal3f(0.0, 0.0, -1.0);
      else
         glNormal3f(0.0, 0.0, 1.0);

      // Draw in the Z plane, at origin, as rotation matrices used in drawing this DL will expect that.
      // top panel
      glTexCoord2f(0.0, 0.0); glVertex3f(+cellRad, +cellRad, z);
      glTexCoord2f(1.0, 0.0); glVertex3f(-cellRad, +cellRad, z);
      glTexCoord2f(1.0, texEdge1); glVertex3f(-cellRad, +holeRad, z);
      glTexCoord2f(0.0, texEdge1); glVertex3f(+cellRad, +holeRad, z);

      // right panel
      glTexCoord2f(0.0, texEdge1); glVertex3f(+cellRad, +holeRad, z);
      glTexCoord2f(texEdge1, texEdge1); glVertex3f(+holeRad, +holeRad, z);
      glTexCoord2f(texEdge1, texEdge2); glVertex3f(+holeRad, -holeRad, z);
      glTexCoord2f(0.0, texEdge2); glVertex3f(+cellRad, -holeRad, z);

      // left panel
      glTexCoord2f(texEdge2, texEdge1); glVertex3f(-holeRad, +holeRad, z);
      glTexCoord2f(1.0, texEdge1); glVertex3f(-cellRad, +holeRad, z);
      glTexCoord2f(1.0, texEdge2); glVertex3f(-cellRad, -holeRad, z);
      glTexCoord2f(texEdge2, texEdge2); glVertex3f(-holeRad, -holeRad, z);

      // bottom panel
      glTexCoord2f(0.0, texEdge2); glVertex3f(+cellRad, -holeRad, z);
      glTexCoord2f(1.0, texEdge2); glVertex3f(-cellRad, -holeRad, z);
      glTexCoord2f(1.0, 1.0); glVertex3f(-cellRad, -cellRad, z);
      glTexCoord2f(0.0, 1.0); glVertex3f(+cellRad, -cellRad, z);
   }

   // inner edges of facade. Here if quads.
   // top edge
   glNormal3f(0.0, -1.0, 0.0);
   glTexCoord2f(texEdge1, texEdge1); glVertex3f(+holeRad, +holeRad, 0);
   glTexCoord2f(texEdge2, texEdge1); glVertex3f(-holeRad, +holeRad, 0);
   glTexCoord2f(texEdge2, texEdge1+th); glVertex3f(-holeRad, +holeRad, th);
   glTexCoord2f(texEdge1, texEdge1+th); glVertex3f(+holeRad, +holeRad, th);

   // bottom edge
   glNormal3f(0.0, 1.0, 0.0);
   glTexCoord2f(texEdge1, texEdge2); glVertex3f(+holeRad, -holeRad, 0);
   glTexCoord2f(texEdge2, texEdge2); glVertex3f(-holeRad, -holeRad, 0);
   glTexCoord2f(texEdge2, texEdge2-th); glVertex3f(-holeRad, -holeRad, th);
   glTexCoord2f(texEdge1, texEdge2-th); glVertex3f(+holeRad, -holeRad, th);

   // left edge
   glNormal3f(1.0, 0.0, 0.0);
   glTexCoord2f(texEdge2-0.02, texEdge1); glVertex3f(+holeRad, +holeRad, 0);
   glTexCoord2f(texEdge2, texEdge1); glVertex3f(+holeRad, +holeRad, th);
   glTexCoord2f(texEdge2, texEdge2); glVertex3f(+holeRad, -holeRad, th);
   glTexCoord2f(texEdge2-0.02, texEdge2); glVertex3f(+holeRad, -holeRad, 0);

   // right edge
   glNormal3f(-1.0, 0.0, 0.0);
   glTexCoord2f(texEdge1, texEdge1); glVertex3f(-holeRad, +holeRad, 0);
   glTexCoord2f(texEdge1+0.02, texEdge1); glVertex3f(-holeRad, +holeRad, th);
   glTexCoord2f(texEdge1+0.02, texEdge2); glVertex3f(-holeRad, -holeRad, th);
   glTexCoord2f(texEdge1, texEdge2); glVertex3f(-holeRad, -holeRad, 0);

   //TODO: outer edges of facade.
   // top edge
   glNormal3f(0.0, 1.0, 0.0);
   glTexCoord2f(0.0, 0.0); glVertex3f(+cellRad, +cellRad - ep, 0);
   glTexCoord2f(1.0, 0.0); glVertex3f(-cellRad, +cellRad - ep, 0);
   glTexCoord2f(1.0, th); glVertex3f(-cellRad, +cellRad - ep, th);
   glTexCoord2f(0.0, th); glVertex3f(+cellRad, +cellRad - ep, th);

   // bottom edge
   glNormal3f(0.0, -1.0, 0.0);
   glTexCoord2f(0.0, th); glVertex3f(+cellRad, -cellRad + ep, 0);
   glTexCoord2f(1.0, th); glVertex3f(-cellRad, -cellRad + ep, 0);
   glTexCoord2f(1.0, 0.0); glVertex3f(-cellRad, -cellRad + ep, th);
   glTexCoord2f(0.0, 0.0); glVertex3f(+cellRad, -cellRad + ep, th);

   // left edge
   glNormal3f(1.0, 0.0, 0.0);
   glTexCoord2f(1.0-th, 0.0); glVertex3f(+cellRad - ep, +cellRad, 0);
   glTexCoord2f(1.0, 0.0); glVertex3f(+cellRad - ep, +cellRad, th);
   glTexCoord2f(1.0, 1.0); glVertex3f(+cellRad - ep, -cellRad, th);
   glTexCoord2f(1.0-th, 1.0); glVertex3f(+cellRad - ep, -cellRad, 0);

   // right edge
   glNormal3f(1.0, 0.0, 0.0);
   glTexCoord2f(0.0, 0.0); glVertex3f(-cellRad + ep, +cellRad, 0);
   glTexCoord2f(th, 0.0); glVertex3f(-cellRad + ep, +cellRad, th);
   glTexCoord2f(th, 1.0); glVertex3f(-cellRad + ep, -cellRad, th);
   glTexCoord2f(0.0, 1.0); glVertex3f(-cellRad + ep, -cellRad, 0);


   glEnd(); // GL_QUADS

   //TODO ###: inner edges of facade. Here if cylinder.

   glEndList(); // facadeDL
}

void setOrthographicProjection() {

	// switch to projection mode
	glMatrixMode(GL_PROJECTION);
	// save previous matrix which contains the 
	//settings for the perspective projection
	glPushMatrix();
	// reset matrix
	glLoadIdentity();
	// set a 2D orthographic projection
	gluOrtho2D(0, xRes, 0, yRes);
	// invert the y axis, down is positive
	glScalef(1, -1, 1);
	// mover the origin from the bottom left corner
	// to the upper left corner
	glTranslatef(0, -yRes, 0);
	glMatrixMode(GL_MODELVIEW);
}

void resetPerspectiveProjection() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}


void celebrateSolution() {
   maze.whenSolved = clock();
   maze.lastSolvedTime = maze.whenSolved - maze.whenEntered;
   if (highScoreList.addScore(maze)) {
      // if this is a new high score (low time), save the high score list.
      highScoreList.save();
      maze.newBest = true;
   }
   else maze.newBest = false;
}
// see http://bytes.com/forum/post832171-3.html regarding CLOCKS_PER_SEC and clock_t type.

// Draw any needed text.
// Does not preserve any previous projection.
void drawText()
{
	// calculate framerate. Thanks to http://nehe.gamedev.net/data/articles/article.asp?article=17
	static int frames = 0;
	static clock_t last_time = 0;
        static char solvingStatus[50];
        int minutes = 0;

	clock_t time_now = clock();
        if (last_time == 0) last_time = time_now;

        if (maze.whenSolved && time_now - maze.whenSolved < howLongShowSolved * CLOCKS_PER_SEC) {
           minutes = maze.lastSolvedTime / (CLOCKS_PER_SEC * 60);
           sprintf(solvingStatus, "SOLVED in %d:%02.2f %s", minutes,
              (maze.lastSolvedTime + 0.0 - (minutes * CLOCKS_PER_SEC * 60)) / CLOCKS_PER_SEC,
              maze.newBest ? " ** New best time! **" : "");
        }
        //else if (maze.hasFoundExit)
        //   sprintf(solvingStatus, "Found exit");
        else if (maze.whenEntered) {
           minutes = (time_now - maze.whenEntered) / (CLOCKS_PER_SEC * 60);
           sprintf(solvingStatus, "Solving: %d:%02.2f", minutes, (time_now - maze.whenEntered + 0.0 - (minutes * CLOCKS_PER_SEC * 60)) / CLOCKS_PER_SEC);
        }
        else solvingStatus[0] = '\0';
        createTextDLs(solvDLOuter, solvingStatus);

	++frames;
	// To update framerate more frequently, lower the right-hand side to e.g. (CLOCKS_PER_SEC / 2)
	if(time_now - last_time > CLOCKS_PER_SEC / 8) {
		// Calculate frames per second
		// debugMsg("time_now: %d; last_time: %d; diff: %d; frames: %d\n", time_now, last_time, time_now - last_time, frames);
		Cam.m_framerate = ((float)frames * CLOCKS_PER_SEC)/(time_now - last_time);
                Cam.m_frametime = (time_now - last_time)/((float)frames * CLOCKS_PER_SEC);
                if (Cam.m_framerate < Cam.m_minframerate) Cam.m_framerate = Cam.m_minframerate; // can't go too low or the following division will make things go wacky.
                Cam.m_framerateAdjust = Cam.m_targetframerate / Cam.m_framerate;
                // createTextDLs(fpsDLOuter, "FPS: %2.2f\nFRA: %2.2f", Cam.m_framerate, Cam.m_framerateAdjust);
                //TODO: make this reflect actual insideness
                createTextDLs(fpsDLOuter, "FPS: %2.2f", Cam.m_framerate);

                last_time = time_now;
		frames = 0;
	}

	// Thanks to: http://glprogramming.com/red/chapter08.html#name1
	// and http://www.lighthouse3d.com/opengl/glut/index.php?bmpfontortho
	setOrthographicProjection();
	glPushMatrix();
	glLoadIdentity();

	if (showFPS) glCallList(fpsDLOuter);
	// else
	if (showHelp) glCallList(helpDLOuter); // createTextDLs(helpText);

        glCallList(solvDLOuter);

	glPopMatrix();
	resetPerspectiveProjection();	
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

	SetupWorld();

	// Create display lists for help and fps
	helpDLOuter = glGenLists(5);
	helpDLInner = helpDLOuter + 1;
	fpsDLOuter = helpDLOuter + 2;
	fpsDLInner = fpsDLOuter + 1;
        solvDLOuter = fpsDLOuter + 2;
        solvDLInner = solvDLOuter + 1;

	// Initialize the help display list. The FPS DL is created every time FPS is recalculated.
	createTextDLs(helpDLOuter, helpText);
	createTextDLs(fpsDLOuter, "FPS: unknown");
        createTextDLs(solvDLOuter, "Solving: n");

        facadeDL = solvDLInner + 1;
        createFacadeDL(facadeDL);

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

	// reset white color (default)
	glColor3f(1.0f, 1.0f, 1.0f);

	//if (firstTime) debugMsg("beginning walls loop\n");

	// debugMsg("zWalls[0][0][0].state = %d\n", maze.zWalls[0][0][0].state);

	// wall1 texture: xWalls
	glBindTexture(GL_TEXTURE_2D, textures[wall1 * numFilters + filter]);

#ifdef DEBUGGING
	//// cylinders for z-axis identification
	//gluCylinder(quadric, 0.2f, 0.2f, 5.0f, 10, 10);
#endif

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i <= maze.w; i++)
		for (int j=0; j < maze.h; j++)
			for (int k=0; k < maze.d; k++) {
				if (maze.xWalls[i][j][k].state == Wall::CLOSED) {
					//if (firstTime) {
					//	debugMsg("drawing x wall %d,%d,%d: ", i, j, k); 
					//}
					maze.xWalls[i][j][k].draw('x');	// draw xWall
				}
			}
	glEnd(); // GL_QUADS

	// wall2 panel texture: zWalls
	glBindTexture(GL_TEXTURE_2D, textures[wall2 * numFilters+filter]);

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < maze.w; i++)
		for (int j=0; j < maze.h; j++)
			for (int k=0; k <= maze.d; k++) {

				if (maze.zWalls[i][j][k].state == Wall::CLOSED) {
					//if (firstTime) {
					//	debugMsg("drawing z wall %d,%d,%d: ", i, j, k); 
					//}
					maze.zWalls[i][j][k].draw('z'); // draw zWall
				}
			}
	glEnd();

	// ground texture
	glBindTexture(GL_TEXTURE_2D, textures[ground * numFilters + filter]);
	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < maze.w; i++)
		for (int j=0; j <= maze.h; j++)
			for (int k=0; k < maze.d; k++) {
				if (maze.yWalls[i][j][k].state == Wall::CLOSED
					&& !maze.yWalls[i][j][k].outsidePositive) {
					//if (firstTime) {
					//	debugMsg("drawing y wall %d,%d,%d\n", i, j, k);
					//}
					maze.yWalls[i][j][k].draw('y'); // draw yWall
				}
			}

	glEnd();

	// roof/ceiling texture
	glBindTexture(GL_TEXTURE_2D, textures[roof * numFilters+filter]);
	// glColor3f(1.0f, 0.5f, 0.5f); // temp for debugging
	//if (firstTime) {
	//	debugMsg("Binding roof texture at %d\n", roof*numFilters+filter);
	//}

	// Process quads
	glBegin(GL_QUADS);

	for (int i=0; i < maze.w; i++)
		for (int j=0; j <= maze.h; j++)
			for (int k=0; k < maze.d; k++) {
				if (maze.yWalls[i][j][k].state == Wall::CLOSED
					&& maze.yWalls[i][j][k].outsidePositive) {
					//if (firstTime) {
					//	debugMsg("drawing y wall roof %d,%d,%d\n", i, j, k);
					//}
					maze.yWalls[i][j][k].draw('y'); // draw yWall
				}
			}

	glEnd();

	// display entrance/exit (may mess up rot/transf matrix)
	maze.ccEntrance.drawExit(maze.entranceWall, true);
	maze.ccExit.drawExit(maze.exitWall, false);

	drawText();											// draw any needed screen text, such as FPS

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
			autopilotMode = false;
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keysDown[wParam] = FALSE;					// If So, Mark It As FALSE
			autopilotMode = false;
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
	delete [] maze.cells;
	delete [] maze.xWalls;
	delete [] maze.yWalls;
	delete [] maze.zWalls;
}

void handleArgs(int argc, LPWSTR *argv) {
   int i;
   int w=8, d=8, h=8, s=3, b=2;

   // skip program names
   for (i = 1; i < argc; i++) {
      if (swscanf(argv[i], L"%dx%dx%d", &w, &h, &d) == 3) {
         maze.w = w; maze.h = h; maze.d = d;
         continue;
      }
      else if (argv[i][0] == '-') {
         switch(argv[i][1]) {
            case 'f':
               fullscreen = TRUE;
               continue;
            //TODO: check for malformed args
            case 's':
               maze.sparsity = _wtoi(argv[++i]);
               continue;
            case 'b':
               maze.branchClustering = _wtoi(argv[++i]);
               continue;
         }
      }
      debugMsg("Unrecognized command-line option: %s\n", argv[i]);
   }

}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

        fullscreen = FALSE;

        LPWSTR *szArglist;
        int nArgs;
        szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

        handleArgs(nArgs, szArglist);

	//// Ask The User Which Screen Mode They Prefer
	//if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
	//{
	//	fullscreen=FALSE;							// Windowed Mode
	//}

        (void)highScoreList.load();

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
				if (autopilotMode) ap->run();
			}
		}
	}

	// Shutdown
	KillGLWindow();										// Kill The Window
	freeResources();
	return ((int)msg.wParam);								// Exit The Program
}

// FIXME: factor out common code, particularly for keys that are not held down, such as SPACE.
// Make an array appKeys[] = { VK_SPACE, 'F', ... }
// then loop through the array.
// Maybe also use portable openGL code instead of windows-specific?
// see http://www.lighthouse3d.com/opengl/glut/index.php?5
// which may require changing all this to use a GLUT main loop.

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
		maze.ccEntrance.standOutside(maze.entranceWall);
	}
	else if (!keysDown[VK_HOME])
	{
		keysStillDown[VK_HOME]=FALSE;
	}

	// reset position / heading / pitch to the end of the maze.
	if (keysDown[VK_END] && !keysStillDown[VK_END]) {
		keysStillDown[VK_END]=TRUE;
		maze.ccExit.standOutside(maze.exitWall);
	}
	else if (!keysDown[VK_END])
	{
		keysStillDown[VK_END]=FALSE;
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

	if (keysDown[VK_RETURN] && !keysStillDown[VK_RETURN])
	{
		// toggle autopilotMode
		keysStillDown[VK_RETURN]=TRUE;
		setAutopilotMode(!autopilotMode);
	}
	else if (!keysDown[VK_RETURN])
	{
		keysStillDown[VK_RETURN]=FALSE;
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
		maze.checkCollisions = !maze.checkCollisions;
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

	// '/?' key: toggle help display
	// Good grief, all we can call this is OEM_2?
	if (keysDown[VK_OEM_2] && !keysStillDown[VK_OEM_2]) {
		keysStillDown[VK_OEM_2]=TRUE;
		showHelp = !showHelp;
		// if (showHelp) showFPS = false;
	}
	else if (!keysDown[VK_OEM_2])
	{
		keysStillDown[VK_OEM_2]=FALSE;
	}

	// 'T' key: toggle fps display
	if (keysDown['T'] && !keysStillDown['T']) {
		keysStillDown['T']=TRUE;
		showFPS = !showFPS;
		// if (showFPS) showHelp = false;
	}
	else if (!keysDown['T'])
	{
		keysStillDown['T']=FALSE;
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
	if (!maze.checkCollisions) {
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

        if (!qcc.isCellPassageSafe()) {
           if (maze.hasFoundExit) {
              maze.hasFoundExit = false;
              celebrateSolution();
           }
           maze.whenEntered = 0;
        }
        else if (qcc == maze.ccEntrance) {
           maze.whenEntered = clock();
           maze.hasFoundExit = false;
        }
        else if (qcc == maze.ccExit && maze.whenEntered) {
           maze.hasFoundExit = true; // solved maze!
        }

	// debugMsg("In collide(<%.2f %.2f %.2f>... ", q.x, q.y, q.z);

	// For each of the six possible walls:
	// - test whether possible wall is within maze
	//   and whether possible wall is a wall
	// - test if q is within wallMargin of wall
	ncc.x = qcc.x - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.x - (qcc.x * maze.cellSize) < maze.wallMargin)) {
		q.x = qcc.x * maze.cellSize + maze.wallMargin;
		result = true;
	} else {
		ncc.x = qcc.x + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.x * maze.cellSize) - q.x < maze.wallMargin) {
			q.x = ncc.x * maze.cellSize - maze.wallMargin;
			result = true;
		}
	}
	ncc.x = qcc.x; // restore it

	ncc.y = qcc.y - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.y - (qcc.y * maze.cellSize) < maze.wallMargin)) {
		q.y = qcc.y * maze.cellSize + maze.wallMargin;
		result = true;
	} else {	
		ncc.y = qcc.y + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.y * maze.cellSize) - q.y < maze.wallMargin) {
			q.y = ncc.y * maze.cellSize - maze.wallMargin;
			result = true;
		}
	}
	ncc.y = qcc.y; // restore it

	ncc.z = qcc.z - 1;
	if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
		&& (q.z - (qcc.z * maze.cellSize) < maze.wallMargin)) {
		q.z = qcc.z * maze.cellSize + maze.wallMargin;
		result = true;
	} else {	
		ncc.z = qcc.z + 1;
		if (qcc.getStateOfWallToSafe(&ncc) == Wall::CLOSED
			&& (ncc.z * maze.cellSize) - q.z < maze.wallMargin) {
			q.z = ncc.z * maze.cellSize - maze.wallMargin;
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

// Turn auto-pilot on or off.
void setAutopilotMode(bool newAP) {
	autopilotMode = newAP;
	if (autopilotMode) {
		// initialize autopilot
		maze.ccEntrance.standOutside(maze.entranceWall);
		ap->addMove(Autopilot::Forward);
	}
}
