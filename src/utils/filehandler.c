//
//  filehandler.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "filehandler.h"
#include "../utils/logging.h"
#include "assert.h"
#include <limits.h> //For SSIZE_MAX
#ifndef WINDOWS
#include <libgen.h>
#endif
#include "string.h"
#include <errno.h>

char *loadFile(const char *fileName, size_t *bytes) {
	FILE *f = fopen(fileName, "rb");
	if (!f) {
		logr(warning, "Can't access '%.*s', error: \"%s (%i)\"\n", (int)strlen(fileName) - 1, fileName, strerror(errno), errno);
		return NULL;
	}
	size_t len = getFileSize(fileName);
	char *buf = malloc(len * sizeof(char));
	fread(buf, sizeof(char), len, f);
	if (ferror(f) != 0) {
		logr(warning, "Error reading file\n");
	} else {
		buf[len] = '\0';
	}
	if (bytes) *bytes = len;
	return buf;
}

void writeFile(const unsigned char *buf, size_t bufsize, const char *filename) {
	FILE* file;
	file = fopen(filename, "wb" );
	char *backupPath = NULL;
	if(!file) {
		backupPath = concatString("./", getFileName(filename));
		file = fopen(backupPath, "wb");
		if (file) {
			char *path = getFilePath(filename);
			logr(warning, "The specified output directory \"%s\" was not writeable, dumping the file in CWD instead.\n", path);
			free(path);
		} else {
			logr(warning, "Neither the specified output directory nor the current working directory were writeable. Image can't be saved. Fix your permissions!");
			return;
		}
	}
	logr(info, "Saving result in \"%s\"\n", backupPath ? backupPath : filename);
	fwrite(buf, 1, bufsize, file);
	fclose(file);
	printFileSize(backupPath ? backupPath : filename);
}

bool isValidFile(char *path) {
	FILE *f = fopen(path, "r");
	if (f) {
		fclose(f);
		return true;
	}
	return false;
}

//Wait for 2 secs and abort if nothing is coming in from stdin
void checkBuf() {
#ifndef WINDOWS
	fd_set set;
	struct timeval timeout;
	int rv;
	FD_ZERO(&set);
	FD_SET(0, &set);
	timeout.tv_sec = 2;
	timeout.tv_usec = 1000;
	rv = select(1, &set, NULL, NULL, &timeout);
	if (rv == -1) {
		logr(error, "Error on stdin timeout\n");
	} else if (rv == 0) {
		logr(error, "No input found after %li seconds. Hint: Try `./bin/c-ray input/scene.json`.\n", timeout.tv_sec);
	} else {
		return;
	}
#endif
}


//TODO: Make these consistent. Now I have to free getFilePath, but not getFileName
/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(char *input) {
	char *fn;
	
	/* handle trailing '/' e.g.
	 input == "/home/me/myprogram/" */
	if (input[(strlen(input) - 1)] == '/')
		input[(strlen(input) - 1)] = '\0';
	
	(fn = strrchr(input, '/')) ? ++fn : (fn = input);
	
	return fn;
}

//For Windows
#define CRAY_PATH_MAX 4096

char *getFilePath(const char *input) {
	char *dir = NULL;
#ifdef WINDOWS
	dir = calloc(256, sizeof(*dir));
	_splitpath_s(input, NULL, 0, dir, sizeof(dir), NULL, 0, NULL, 0);
#else
	char *inputCopy = copyString(input);
	dir = copyString(dirname(inputCopy));
	free(inputCopy);
#endif
	char *final = concatString(dir, "/");
	free(dir);
	return final;
}

#define chunksize 1024
//Get scene data from stdin and return a pointer to it
char *readStdin(size_t *bytes) {
	checkBuf();
	
	char chunk[chunksize];
	
	size_t bufSize = 1;
	char *buf = malloc(chunksize * sizeof(*buf));
	if (!buf) {
		logr(error, "Failed to malloc stdin buffer\n");
		return NULL;
	}
	buf[0] = '\0';
	while (fgets(chunk, chunksize, stdin)) {
		char *old = buf;
		bufSize += strlen(chunk);
		buf = realloc(buf, bufSize);
		if (!buf) {
			logr(error, "Failed to realloc stdin buffer\n");
			free(old);
			return NULL;
		}
		strcat(buf, chunk);
	}
	
	if (ferror(stdin)) {
		logr(error, "Failed to read from stdin\n");
		free(buf);
		return NULL;
	}
	
	if (bytes) *bytes = bufSize - 1;
	return buf;
}

char *humanFileSize(unsigned long bytes) {
	unsigned long kilobytes, megabytes, gigabytes, terabytes; // <- Futureproofing?!
	kilobytes = bytes / 1000;
	megabytes = kilobytes / 1000;
	gigabytes = megabytes / 1000;
	terabytes = gigabytes / 1000;
	
	char *buf = calloc(64, sizeof(*buf));
	
	if (gigabytes > 1000) {
		sprintf(buf, "%ldTB", terabytes);
	} else if (megabytes > 1000) {
		sprintf(buf, "%ldGB", gigabytes);
	} else if (kilobytes > 1000) {
		sprintf(buf, "%ldMB", megabytes);
	} else if (bytes > 1000) {
		sprintf(buf, "%ldKB", kilobytes);
	} else {
		sprintf(buf, "%ldB", bytes);
	}
	return buf;
}

void printFileSize(const char *fileName) {
	//We determine the file size after saving, because the lodePNG library doesn't have a way to tell the compressed file size
	//This will work for all image formats
	unsigned long bytes = getFileSize(fileName);
	char *sizeString = humanFileSize(bytes);
	logr(info, "Wrote %s to file.\n", sizeString);
	free(sizeString);
}

size_t getFileSize(const char *fileName) {
	FILE *file = fopen(fileName, "r");
	if (!file) return 0;
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fclose(file);
	return size;
}
