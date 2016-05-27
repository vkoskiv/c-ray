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

//Saves the data from a given array to a PPM file.
void saveImageFromArray(const char *filename, const unsigned char *imgdata, unsigned width, unsigned height);

//Saves the data from a given array to a BMP file.
void saveBmpFromArray(const char *filename, const unsigned char *imgData, unsigned width, unsigned height);

//Encodes data from a given array to a BMP file.
void encodePNG(const char *filename, const unsigned char *imgData, unsigned width, unsigned height);

#endif /* defined(__C_Ray__filehandler__) */
