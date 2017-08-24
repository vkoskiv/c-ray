//
//  filehandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct scene;

enum fileType {
	bmp,
	png
};

struct dimensions {
	int height;
	int width;
};

struct outputImage {
	struct dimensions size;
	enum fileType fileType;
	char *filePath;
	unsigned char *imgData;
};

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes image data to file
void writeImage(const char *filePath, unsigned char *imgData, enum fileType type, int currentFrame, struct dimensions imgSize);
