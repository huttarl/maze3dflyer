#ifndef __HighScoreList_h__
#define __HighScoreList_h__

#include <map>
#include <string>
#include "Maze3D.h"

using namespace std;

struct compareDims
{
   // comparison function for dims
   // "The key comparison function, a Strict Weak Ordering whose argument type is key_type;
   // it returns true if its first argument is less than its second argument, and false otherwise."
   // We want to order maze dimensions basically by their difficulty, but also make the score list
   // visually scannable. So we order by greatest dimension, then 2nd greatest, then smallest,
   // and then by sparsity.
   bool operator()(string dims1, string dims2) const
   {
      int w1, h1, d1, s1;
      int w2, h2, d2, s2;
      if (sscanf(dims1.c_str(), "%dx%dx%d/%d", &w1, &h1, &d1, &s1) < 4 ||
          sscanf(dims2.c_str(), "%dx%dx%d/%d", &w2, &h2, &d2, &s2) < 4)
         return (dims1 < dims2);
      if (w1 < w2) return true;
      else if (w1 > w2) return false;
      if (h1 < h2) return true;
      else if (h1 > h2) return false;
      if (d1 < d2) return true;
      else if (d1 > d2) return false;
      // Note, a maze with greater sparsity is less complex, so reverse the comparison.
      if (s1 > s2) return true;
      else return false;
   }
};


class HighScoreList {
   map<string, float, compareDims> highScoreMap;
   map<string, float, compareDims>::iterator p;
public:
   // Add new score, if it is a new record, for the given dims.
   // dims = 'wxhxd/s'. t = time in seconds, to the nearest 100th.
   // Return true if new record established.
   bool addScore(char *dims, float t);
   bool addScore(Maze3D &maze);
   float getHighScore(char *dims);
   int HighScoreList::getPosition(char *dims);
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
   char *toString(Maze3D &maze, int linesAllowed = 20);

   static void complexityStats(void);
};

extern HighScoreList highScoreList;

#endif // __HighScoreList_h__
