#include "Wall.h"
#include "Maze3D.h"
#include "maze3dflyer.h"

// Within a glBegin/glEnd, draw a quad for this wall.
void Wall::draw(char dir) {
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
void Wall::drawExit(int x, int y, int z, bool isEntrance) {
	Vertex *qv = quad.vertices;
	GLfloat cx = qv[0].x + Maze3D::cellSize * 0.5f,
		cy = qv[0].y + Maze3D::cellSize * 0.5f,
		cz = qv[0].z + Maze3D::cellSize * 0.5f; // center of disc
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
	maze.exitRot += 1.25f;
	glRotatef(isEntrance ? maze.exitRot : -maze.exitRot, 0.0f, 0.0f, 1.0f);

	// gluDisk(quadric, innerRadius, outerRadius, slices, loops)
	gluDisk(quadric, Maze3D::cellSize * 0.4f, Maze3D::cellSize * 0.5f, 5, 3);

	glPopMatrix();
}
