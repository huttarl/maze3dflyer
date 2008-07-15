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
   // dims = 'wxhxd/s'. t = time in seconds, to the nearest 100th.
   // Return true if new record established.
   bool addScore(char *dims, float t);
   bool addScore(Maze3D &maze);
   float getHighScore(char *dims);
   bool save(void);
   bool load(void);
   char *filepath;
   HighScoreList() { filepath = "maze3dflyer_scores.txt"; }
   // makeDims: convert the maze parameters to a single string
   static char *dims(Maze3D &maze, bool normalize = false) {
      static char buf[16];
      int i = maze.w, j = maze.h, k = maze.d;
      // sort dimensions by size, so that 6x2x10 has the same best score slot as 10x6x2.
      if (normalize) {
         if (i < j) swap(i, j);
         if (j < k) swap(j, k);
         if (i < j) swap(i, j);
      }
      sprintf(buf, "%dx%dx%d/%d", i, j, k, maze.sparsity);
      return buf;
   }

   static char *formatTime(float t, bool rightJustify = true);
   // return pointer to a static buffer with a display of scores
   char *toString(Maze3D &maze);

   static void complexityStats(void);
};

extern HighScoreList highScoreList;

#endif // __HighScoreList_h__
