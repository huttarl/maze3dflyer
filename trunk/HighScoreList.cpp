#include <iostream>
#include <iomanip>
#include <fstream>

#include "maze3dflyer.h"
#include "HighScoreList.h"

bool HighScoreList::addScore(char *dims, float t) {
   debugMsg("Adding time: '%s' -> %f\n", dims, t);
   p = highScoreMap.find(dims);
   if (p == highScoreMap.end() ||
      p->second > t) {
      highScoreMap[dims] = t;
      return true;
   }
   else return false;
}

bool HighScoreList::addScore(Maze3D &maze) {
   return addScore(makeDims(maze.w, maze.h, maze.d, maze.sparsity), ((maze.lastSolvedTime + 0.0) / CLOCKS_PER_SEC));
}

// get high score for a given map dimension
float HighScoreList::getHighScore(char *dims) {
   p = highScoreMap.find(dims);
   if (p == highScoreMap.end()) return -1.0;
   else return p->second;
}

// save high scores to a file (using YAML)
bool HighScoreList::save(void) {
   ofstream fp(filepath, ios::out);

   if (!fp) {
      debugMsg("Can't open output file %s\n", filepath);
      return false;
   }

   fp << "# High scores for maze3dflyer" << endl;
   fp << "# " << (unsigned int)(highScoreMap.size()) << " entries." << endl; // may not be used by this program but useful for somebody.

   for (p = highScoreMap.begin(); p != highScoreMap.end(); p++) {
      fp << p->first << " " << setiosflags(ios::fixed) << setprecision(2) << p->second << endl;
   }

   fp.close();
   return true;
}

// load high scores from a file (using YAML)
bool HighScoreList::load(void) {
   int count;
   char line[256], dims[16];
   float time;

   ifstream fp(filepath, ios::in);

   if (!fp) {
      debugMsg("Can't open input file %s\n", filepath);
      return false;
   }

   fp.ignore(256, '\n'); // ignore "# High scores for maze3dflyer\n" 
   fp.ignore(3, ' '); // ignore "# "
   fp >> count;
   fp.ignore(256, '\n'); // ignore " entries.\n"

   highScoreMap.clear();

   while (fp >> dims >> time) {
      // sscanf(line, "- [%s, %f]", dims, &time);
      debugMsg("Read score line: '%s' -> %f\n", dims, time);
      highScoreMap[dims] = time;
   }

   if (highScoreMap.size() != count)
      debugMsg("Expected %d scores; found %d\n", count, highScoreMap.size());

   fp.close();
   return true;
}

