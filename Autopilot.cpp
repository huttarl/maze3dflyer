// Autopilot class
// Copyright (c) 2008 by Lars Huttar

#include "Autopilot.h"
#include "maze3dflyer.h"

// Carry out next planned move, if ready; and plan ahead further, if ready.

void Autopilot::run() {
	planAhead();
	// currentMove == None means that the last executing move is finished; time to start the next one.
	if (currentMove == None) {
		currentMove = popMove();
		// #todo: begin move
	} else {
		// continue current move
		switch(currentMove) {
			case Forward:
				// if next move is not forward and we're almost to the next juncture, start decelerating.
				// else accelerate (up to max)
				break;
			case TurnUp:
				// if next move is not TurnUp and we're almost to the next juncture, start decelerating.
				// else accelerate (up to max)
				break;
		}
	}
}

void Autopilot::planAhead() {
}

Autopilot::APMove Autopilot::popMove() {
	APMove result;
	if (qStart != qEnd)
		result = moveQueue[qStart++];
	else {
		// errorMsg("Autopilot tried to pop empty move queue\n");
		result = None;
	}
	if (qStart >= qMaxSize) qStart = 0;
	return result;
}
