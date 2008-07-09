#ifndef __HighScoreList_h__
#define __HighScoreList_h__

#include <map>
#include <string>
#include "Maze3D.h"

using namespace std;

class HighScoreList {
   map<string, float> highScoreMap;
   map<string, float>::iterator p;
public:
   // Add new score, if it is a new record, for the given dims.
   // dims = 'wxhxdxs'. t = time in seconds, to the nearest 100th.
   // Return true if new record established.
   bool addScore(char *dims, float t);
   bool addScore(Maze3D &maze);
   float getHighScore(char *dims);
   bool save(void);
   bool load(void);
   char *filepath;
   HighScoreList() { filepath = "maze3dflyer_scores.txt"; }
   // makeDims: convert the maze parameters to a single string
   static char *makeDims(Maze3D &maze) {
      static char buf[16];
      sprintf(buf, "%dx%dx%d/%d", maze.w, maze.h, maze.d, maze.sparsity);
      return buf;
   }
   // return pointer to a static buffer with a display of scores
   char *toString(Maze3D &maze);
};

extern HighScoreList highScoreList;

#endif // __HighScoreList_h__
