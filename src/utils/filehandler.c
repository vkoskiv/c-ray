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
	char *buf = malloc(len + 1 * sizeof(char));
	fread(buf, sizeof(char), len, f);
	if (ferror(f) != 0) {
		logr(warning, "Error reading file\n");
	} else {
		buf[len] = '\0';
	}
	if (bytes) *bytes = len;
	return buf;
}

void writeFile(const unsigned char *buf, size_t bufsize, const char *filePath) {
	FILE *file = fopen(filePath, "wb" );
	char *backupPath = NULL;
	if(!file) {
		char *name = getFileName(filePath);
		backupPath = concatString("./", name);
		free(name);
		file = fopen(backupPath, "wb");
		if (file) {
			char *path = getFilePath(filePath);
			logr(warning, "The specified output directory \"%s\" was not writeable, dumping the file in CWD instead.\n", path);
			free(path);
		} else {
			logr(warning, "Neither the specified output directory nor the current working directory were writeable. Image can't be saved. Fix your permissions!");
			return;
		}
	}
	logr(info, "Saving result in \"%s\"\n", backupPath ? backupPath : filePath);
	fwrite(buf, 1, bufsize, file);
	fclose(file);
	printFileSize(backupPath ? backupPath : filePath);
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

/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(const char *input) {
	//FIXME: We're doing two copies here, maybe just rework the algorithm instead.
	char *copy = copyString(input);
	char *fn;
	
	/* handle trailing '/' e.g.
	 input == "/home/me/myprogram/" */
	if (copy[(strlen(copy) - 1)] == '/')
		copy[(strlen(copy) - 1)] = '\0';
	
	(fn = strrchr(copy, '/')) ? ++fn : (fn = copy);
	
	char *ret = copyString(fn);
	free(copy);
	
	return ret;
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
	float kilobytes, megabytes, gigabytes, terabytes, petabytes, exabytes, zettabytes, yottabytes; // <- Futureproofing?!
	kilobytes  = bytes      / 1000.0f;
	megabytes  = kilobytes  / 1000.0f;
	gigabytes  = megabytes  / 1000.0f;
	terabytes  = gigabytes  / 1000.0f;
	petabytes  = terabytes  / 1000.0f;
	exabytes   = petabytes  / 1000.0f;
	zettabytes = exabytes   / 1000.0f;
	yottabytes = zettabytes / 1000.0f;
	
	// Okay, okay. In reality, this never gets even close to a zettabyte,
	// it'll overflow at around 18 exabytes.
	// I *did* get it to go to yottabytes using __uint128_t, but that's
	// not in C99. Maybe in the future.
	
	char *buf = calloc(64, sizeof(*buf));
	
	if (zettabytes > 1000) {
		sprintf(buf, "%.02fYB", yottabytes);
	} else if (exabytes > 1000) {
		sprintf(buf, "%.02fZB", zettabytes);
	} else if (petabytes > 1000) {
		sprintf(buf, "%.02fEB", exabytes);
	} else if (terabytes > 1000) {
		sprintf(buf, "%.02fPB", petabytes);
	} else if (gigabytes > 1000) {
		sprintf(buf, "%.02fTB", terabytes);
	} else if (megabytes > 1000) {
		sprintf(buf, "%.02fGB", gigabytes);
	} else if (kilobytes > 1000) {
		sprintf(buf, "%.02fMB", megabytes);
	} else if (bytes > 1000) {
		sprintf(buf, "%.02fkB", kilobytes);
	} else {
		sprintf(buf, "%lldB", bytes);
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
