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

//For 24 bit buffer
void blit(struct texture *t, struct color *c, unsigned int x, unsigned int y);

//For internal render buffer
void blitDouble(double *buf, int width, int height, struct color *c, unsigned int x, unsigned int y);

char *loadFile(char *inputFileName);

char *readStdin(void);

//Load a PNG texture
struct texture *newTexture(char *filePath);

void copyString(const char *source, char **destination);

void freeImage(struct texture *image);
