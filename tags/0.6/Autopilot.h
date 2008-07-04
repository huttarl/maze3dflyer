// Autopilot class
// Copyright (c) 2008 by Lars Huttar

#ifndef AUTOPILOT_H
#define AUTOPILOT_H


#include "Maze3D.h"
#include "glCamera.h"

class Autopilot {
public:
	enum APMove { None, Forward, Reverse, TurnUp, TurnDown, TurnLeft, TurnRight };
private:
	const static int qMaxSize = Maze3D::wMax + Maze3D::hMax + Maze3D::dMax; // max number of moves planned ahead
	Maze3D maze;
	glCamera cam;
	// To do: should make queue items a structure that includes new cellCoord expected after each move.
	enum APMove currentMove, moveQueue[qMaxSize];
	int qStart, qEnd;
	int qSize() { return (qEnd + qMaxSize - qStart) % qMaxSize; }
	// Decide on future moves.
	void planAhead();

public:
	void init(Maze3D _maze, glCamera _cam) {
		maze = _maze;
		cam = _cam;
		qStart = 0;
		qEnd = 0;
		currentMove = None;
	}
	void addMove(APMove m) {
		moveQueue[qEnd++] = m;
		if (qEnd >= qMaxSize) qEnd = 0;
	}
	APMove popMove();
	// Carry out next planned move, if ready; and plan ahead further, if ready.
	void run();
};

#endif // AUTOPILOT_H
