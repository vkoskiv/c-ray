//
//  filehandler.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct scene;
struct renderer;

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
	char *fileName;
	int count;
	unsigned char *data;
};

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes image data to file
void writeImage(struct renderer *r);

void freeImage(struct outputImage *image);
