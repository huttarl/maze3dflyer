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
   return addScore(dims(maze), ((maze.lastSolvedTime + 0.0) / CLOCKS_PER_SEC));
}

// get high score for a given map dimension
float HighScoreList::getHighScore(char *dims) {
   p = highScoreMap.find(dims);
   if (p == highScoreMap.end()) return -1.0;
   else return p->second;
}

// save high scores to a file
bool HighScoreList::save(void) {
   ofstream fp(filepath, ios::out);

   if (!fp) {
      debugMsg("Can't open output file %s\n", filepath);
      return false;
   }

   fp << "# High scores for maze3dflyer" << endl;
   // since this number will be parsed back in, don't put it in a "comment".
   fp << (unsigned int)(highScoreMap.size()) << " # Number of entries." << endl; // may not be used by this program but useful for somebody.

   for (p = highScoreMap.begin(); p != highScoreMap.end(); p++) {
      fp << p->first << " " << setiosflags(ios::fixed) << setprecision(2) << p->second << endl;
   }

   fp.close();
   return true;
}

char *HighScoreList::formatTime(float t, bool rightJustify) {
   static char buf[20];
   int minutes;
   if (t < 0.0) return "n/a";
   else {
      minutes = t / 60.0;
      sprintf(buf, rightJustify ? "%6d:%05.2f" : "%d:%05.2f", minutes, t - (minutes * 60.0));
      return buf;
   }
}

char *HighScoreList::toString(Maze3D &maze) {
   static char buf[2048], line[30];
   int chars = 0;
   char *curDims = dims(maze);
   char *s = buf + sprintf(buf, "     Maze size:   Best time:\n\n");

   for (p = highScoreMap.begin(); p != highScoreMap.end(); p++) {
      chars = sprintf(line, "%s%11s %s\n",
         !strcmp(p->first.c_str(), curDims) ? "-> " : "   ",
         p->first.c_str(), formatTime(p->second));
      if (chars > 0) {
         if (s - buf + chars >= sizeof(buf)) {
            debugMsg("Line '%s' would overflow high score list buffer! %d >= %d\n",
               line, s - buf, sizeof(buf));
            return buf; // don't overflow the buffer
         }
         strcpy(s, line);
         s += chars;
      } // else should process failure in sprintf...
   }
   // debugMsg("High score list follows:\n%s\n", buf);
   return buf;
}

// load high scores from a file
bool HighScoreList::load(void) {
   int count;
   char dims[16];
   float time;

   ifstream fp(filepath, ios::in);

   if (!fp) {
      debugMsg("Can't open input file %s\n", filepath);
      return false;
   }

   //TODO: be a little more flexible; ignore lines that are blank and text after '#'.
   // Don't require the hs file to have exactly one initial comment line.
   fp.ignore(256, '\n'); // ignore "# High scores for maze3dflyer\n" 
   fp >> count;
   fp.ignore(256, '\n'); // ignore " # Number of entries.\n"

   highScoreMap.clear();

   while (fp >> dims >> time) {
      // sscanf(line, "- [%s, %f]", dims, &time);
      // debugMsg("Read score line: '%s' -> %f\n", dims, time);
      highScoreMap[dims] = time;
   }

   if (highScoreMap.size() != count)
      debugMsg("Expected %d scores; found %d\n", count, highScoreMap.size());

   fp.close();
   return true;
}

// only intended for developer use
void HighScoreList::complexityStats(void) {
   // Accumulate stats about complexity of maze:
   {
      char *fn = "mazeComplexityStats.txt";
      int volume = maze.w * maze.h * maze.d;
      ofstream fp(fn, ios::app);

      if (!fp) {
         debugMsg("Can't open output file %s\n", fn);
         return;
      }
      fp << "numPassageCells: " << maze.numPassageCells << " of " << volume <<
         " (" << maze.w << "x" << maze.h << "x" << maze.d << "/" << maze.sparsity << "). Prediction: " << 
         int(((maze.sparsity - 1) * 3) * (volume + 0.0) / (maze.sparsity * maze.sparsity * maze.sparsity)) << endl;
   }
}

