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
#include "renderer.h"

typedef enum {
	loadModeNormal,
	loadModeTarga
}loadMode;

typedef enum {
	saveModeNormal,
	saveModeTarga,
	saveModeNone
}saveMode;

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes an outputfile and returns the path it was written to
void writeImage(scene *worldScene);

#endif /* defined(__C_Ray__filehandler__) */
