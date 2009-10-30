#ifndef __Image_h__
#define __Image_h__

// #include <gl/glut.h> // causes error in redefinition of exit()
typedef unsigned int GLuint;

// An instance of an image hung on a wall somewhere.
class Image {
public:
   GLuint textureId;
   int width, height; // original dimensions, in pixels

   void inline init(GLuint txId, int w, int h) { textureId = txId; width = w; height = h; }
};

#endif // __Image_h__
