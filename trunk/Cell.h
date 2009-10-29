#ifndef __Cell_h__
#define __Cell_h__

class Picture;

class Cell {
public:
	enum CellState { uninitialized, passage, forbidden } state;
	// state = uninitialized: This cell has not been visited yet.
	// state = passage: This cell has been carved out; it is "passageway".
	// state = forbidden: true iff this cell is too close to other passage cells and so cannot be passage.
        bool isOnSolutionRoute;
        // index of prize contained in this cell, if any. -1 means none.
        short iPrize;
        // pointer to picture on wall of this cell, if any. NULL means none.
        Picture *picture;

        Cell() { state = uninitialized; iPrize = -1; picture = (Picture *)0; }
	static CellState getCellState(int x, int y, int z);
};

#endif // __Cell_h__
