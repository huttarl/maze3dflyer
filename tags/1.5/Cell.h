#ifndef __Cell_h__
#define __Cell_h__

class Cell {
public:
	enum CellState { uninitialized, passage, forbidden } state;
	// state = uninitialized: This cell has not been visited yet.
	// state = passage: This cell has been carved out; it is "passageway".
	// state = forbidden: true iff this cell is too close to other passage cells and so cannot be passage.
        bool isOnSolutionRoute;
        short iPrize;
        Cell() { state = uninitialized; iPrize = -1; }
	static CellState getCellState(int x, int y, int z);
};

#endif // __Cell_h__
