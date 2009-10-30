#ifndef __Prize_h__
#define __Prize_h__

class Prize {
public:
   // has this prize been grabbed yet? (on current level)
   boolean taken;
   CellCoord where;
   // image; texture?
   // string name;
   Prize() {
      taken = false;
      // image =
      // name =
   }
};

#endif // __Prize_h__
