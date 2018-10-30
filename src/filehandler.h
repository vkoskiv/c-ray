//
//  filehandler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
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

struct image {
	struct dimensions size;
	enum fileType fileType;
	char *filePath;
	char *fileName;
	int count;
	unsigned char *data;
};

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

size_t getDelim(char **lineptr, size_t *n, int delimiter, FILE *stream);
char *loadFile(char *filePath);

struct image *loadImage(char *filePath);

//Writes image data to file
void writeImage(struct renderer *r);

void freeImage(struct image *image);

void copyString(const char *source, char **destination);
char *getFileName(char *input);
