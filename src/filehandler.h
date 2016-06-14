//
//  filehandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__filehandler__
#define __C_Ray__filehandler__

#include "includes.h"
#include "scene.h"

//Saves the data from a given array to a PPM file.
void saveImageFromArray(const char *filename, const unsigned char *imgdata, unsigned width, unsigned height);

//Saves the data from a given array to a BMP file.
void saveBmpFromArray(const char *filename, const unsigned char *imgData, unsigned width, unsigned height);

//Encodes data from a given array to a PNG file.
void encodePNGFromArray(const char *filename, const unsigned char *imgData, unsigned width, unsigned height);

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes an outputfile and returns the path it was written to
char *writeImage(int currentFrame, world *worldScene, unsigned char *imgData);

#endif /* defined(__C_Ray__filehandler__) */
