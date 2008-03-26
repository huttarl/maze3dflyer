#ifndef JPEG_H
#define JPEG_H

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(lib, "glaux.lib")
#pragma comment(lib, "lib/jpeg.lib")


#include <windows.h>
#include <windowsx.h>
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <stdio.h>
#include <stdlib.h>


//////////////////////////////////////
//The Global Variables
//////////////////////////////////////
extern	HDC			hDC;			// Device Context
extern	HGLRC		hRC;			// Permanent Rendering Context
extern	HWND		hWnd;			// Holds Our Window Handle
extern	HINSTANCE	hInstance;		// Holds The Instance Of The Application

#include "include/jpeglib.h"



tImageJPG *Load_JPEG(const char *strfilename);

void Decompress_JPEG(jpeg_decompress_struct* cInfo, tImageJPG *pImgData);

void JPEG_Texture(UINT textureArray[], LPSTR strFileName, int ID);



#endif



// www.morrowland.com
// apron@morrowland.com