//
//  filehandler.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
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

struct texture {
	enum fileType fileType;
	char *filePath;
	char *fileName;
	int count;
	unsigned char *data;
	unsigned int *width;
	unsigned int *height;
};

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes image data to file
void writeImage(struct renderer *r);

char *loadFile(char *inputFileName);

//Load a PNG texture
struct texture *newTexture(char *filePath);

void copyString(const char *source, char **destination);

void freeImage(struct texture *image);
