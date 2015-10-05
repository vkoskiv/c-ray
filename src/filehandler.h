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
void saveImageFromArray(char *filename, unsigned char *imgdata, int width, int height);

//Saves the data from a given array to a BMP file.
void saveBmpFromArray(char *filename, unsigned char *imgData, int width, int height);

#endif /* defined(__C_Ray__filehandler__) */
