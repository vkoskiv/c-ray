//
//  filehandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct scene;

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
void writeImage(struct scene *worldScene);
