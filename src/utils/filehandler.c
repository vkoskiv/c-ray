//
//  filehandler.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../libraries/asprintf.h"
#include "../includes.h"
#include "filehandler.h"

#include "../datatypes/camera.h"
#include "../datatypes/scene.h"
#include "../renderer/renderer.h"
#include "../utils/logging.h"
#include "../datatypes/texture.h"

//Prototypes for internal functions
int getFileSize(char *fileName);
size_t getDelim(char **lineptr, size_t *n, int delimiter, FILE *stream);

void saveBmpFromArray(const char *filename, unsigned char *imgData, int width, int height) {
	
	//Apparently BMP is BGR, whereas C-ray's internal buffer is RGB (Like it should be)
	//So we need to convert the image data before writing to file.
	
	unsigned char *bgrData = calloc(3 * width * height, sizeof(unsigned char));
	
	//FIXME: For some reason we can't access the 0 of X and Y on imgdata. So now BMP images have 1 black row on left and top edges...
	for (int y = 1; y < height; y++) {
		for (int x = 1; x < width; x++) {
			bgrData[(x + (height - y) * width) * 3 + 0] = imgData[(x + (height - y) * width) * 3 + 2];
			bgrData[(x + (height - y) * width) * 3 + 1] = imgData[(x + (height - y) * width) * 3 + 1];
			bgrData[(x + (height - y) * width) * 3 + 2] = imgData[(x + (height - y) * width) * 3 + 0];
		}
	}
	
	int i;
	int error;
	FILE *f;
	int filesize = 54 + 3 * width * height;
	
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
	unsigned char bmppadding[3] = {0,0,0};
	
	//Create header with filesize data
	bmpfileheader[2] = (unsigned char)(filesize    );
	bmpfileheader[3] = (unsigned char)(filesize>> 8);
	bmpfileheader[4] = (unsigned char)(filesize>>16);
	bmpfileheader[5] = (unsigned char)(filesize>>24);
	
	//create header width and height info
	bmpinfoheader[ 4] = (unsigned char)(width    );
	bmpinfoheader[ 5] = (unsigned char)(width>>8 );
	bmpinfoheader[ 6] = (unsigned char)(width>>16);
	bmpinfoheader[ 7] = (unsigned char)(width>>24);
	
	bmpinfoheader[ 8] = (unsigned char)(height    );
	bmpinfoheader[ 9] = (unsigned char)(height>>8 );
	bmpinfoheader[10] = (unsigned char)(height>>16);
	bmpinfoheader[11] = (unsigned char)(height>>24);
	
	f = fopen(filename,"wb");
	error = (unsigned int)fwrite(bmpfileheader,1,14,f);
	if (error != 14) {
		logr(warning, "Error writing BMP file header data\n");
	}
	error = (unsigned int)fwrite(bmpinfoheader,1,40,f);
	if (error != 40) {
		logr(warning, "Error writing BMP info header data\n");
	}
	
	for (i = 1; i <= height; i++) {
		error = (unsigned int)fwrite(bgrData+(width*(height - i)*3),3,width,f);
		if (error != width) {
			logr(warning, "Error writing image line to BMP\n");
		}
		error = (unsigned int)fwrite(bmppadding,1,(4-(width*3)%4)%4,f);
		if (error != (4-(width*3)%4)%4) {
			logr(warning, "Error writing BMP padding data\n");
		}
	}
	fclose(f);
}

void encodePNGFromArray(const char *filename, unsigned char *imgData, int width, int height) {
	unsigned error = lodepng_encode24_file(filename, imgData, width, height);
	if (error) logr(warning, "error %u: %s\n", error, lodepng_error_text(error));
}

char *loadFile(char *inputFileName) {
	FILE *f = fopen(inputFileName, "rb");
	if (!f) {
		logr(warning, "No file found at %s", inputFileName);
		return NULL;
	}
	char *buf = NULL;
	size_t len;
	size_t bytesRead = getDelim(&buf, &len, '\0', f);
	if (bytesRead != -1) {
		logr(info, "%zi bytes of input JSON loaded from file, parsing.\n", bytesRead);
	} else {
		logr(warning, "Failed to read input JSON from %s", inputFileName);
		return NULL;
	}
	return buf;
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
		logr(error, "No input found after %i seconds. Hint: Try `./bin/c-ray input/scene.json`.\n", timeout.tv_sec);
	} else {
		return;
	}
#endif
}

#define chunksize 1024
//Get scene data from stdin and return a pointer to it
char *readStdin() {
	checkBuf();
	
	char chunk[chunksize];
	
	size_t bufSize = 1;
	char *buf = malloc(chunksize * sizeof(char));
	if (!buf) {
		logr(error, "Failed to malloc stdin buffer\n");
	}
	buf[0] = '\0';
	while (fgets(chunk, chunksize, stdin)) {
		char *old = buf;
		bufSize += strlen(chunk);
		buf = realloc(buf, bufSize);
		if (!buf) {
			free(old);
			logr(error, "Failed to realloc stdin buffer\n");
		}
		strcat(buf, chunk);
	}
	
	if (ferror(stdin)) {
		free(buf);
		logr(error, "Failed to read from stdin\n");
	}
	
	logr(info, "%zi bytes of input JSON loaded from stdin, parsing.\n", bufSize-1);
	
	return buf;
}

//FIXME: Move this to a better place
bool stringEquals(const char *s1, const char *s2) {
	if (strcmp(s1, s2) == 0) {
		return true;
	} else {
		return false;
	}
}
//FIXME: Move this to a better place
bool stringContains(const char *haystack, const char *needle) {
	if (strstr(haystack, needle) == NULL) {
		return false;
	} else {
		return true;
	}
}

void printFileSize(char *fileName) {
	//We determine the file size after saving, because the lodePNG library doesn't have a way to tell the compressed file size
	//This will work for all image formats
	long bytes, kilobytes, megabytes, gigabytes, terabytes; // <- Futureproofing?!
	bytes = getFileSize(fileName);
	kilobytes = bytes / 1000;
	megabytes = kilobytes / 1000;
	gigabytes = megabytes / 1000;
	terabytes = gigabytes / 1000;
	
	if (gigabytes > 1000) {
		logr(info, "Wrote %ldTB to file.\n", terabytes);
	} else if (megabytes > 1000) {
		logr(info, "Wrote %ldGB to file.\n", gigabytes);
	} else if (kilobytes > 1000) {
		logr(info, "Wrote %ldMB to file.\n", megabytes);
	} else if (bytes > 1000) {
		logr(info, "Wrote %ldKB to file.\n", kilobytes);
	} else {
		logr(info, "Wrote %ldB to file.\n", bytes);
	}
}

void writeImage(struct renderer *r) {
	switch (r->mode) {
		case saveModeNormal: {
			//Save image data to a file
			char *buf = NULL;
			
			if (r->image->fileType == bmp){
				asprintf(&buf, "%s%s_%04d.bmp", r->image->filePath, r->image->fileName, r->image->count);
				logr(info, "Saving result in \"%s\"\n", buf);
				saveBmpFromArray(buf, r->image->data, *r->image->width, *r->image->height);
			} else if (r->image->fileType == png){
				asprintf(&buf, "%s%s_%04d.png", r->image->filePath, r->image->fileName, r->image->count);
				logr(info, "Saving result in \"%s\"\n", buf);
				encodePNGFromArray(buf, r->image->data, *r->image->width, *r->image->height);
			}
			printFileSize(buf);
#ifdef __APPLE__
			//If on macOS, we can run the `open` command to display the finished render using Preview.app
			//char *buf2 = NULL;
			//asprintf(&buf2, "open %s", buf);
			//system(buf2);
			//free(buf2);
#endif
			free(buf);
		}
		break;
		case saveModeNone:
			logr(info, "Abort pressed, image won't be saved.\n");
			break;
		default:
			break;
	}
	
}

//For Windows support, we need our own getdelim()
#if defined(_WIN32) || defined(__linux__)
#define	LONG_MAX	2147483647L	/* max signed long */
#endif
#define	SSIZE_MAX	LONG_MAX	/* max value for a ssize_t */
size_t getDelim(char **lineptr, size_t *n, int delimiter, FILE *stream) {
	char *buf, *pos;
	int c;
	size_t bytes;
	
	if (lineptr == NULL || n == NULL) {
		return 0;
	}
	if (stream == NULL) {
		return 0;
	}
	
	/* resize (or allocate) the line buffer if necessary */
	buf = *lineptr;
	if (buf == NULL || *n < 4) {
		buf = (char*)realloc(*lineptr, 128);
		if (buf == NULL) {
			/* ENOMEM */
			return 0;
		}
		*n = 128;
		*lineptr = buf;
	}
	
	/* read characters until delimiter is found, end of file is reached, or an
	 error occurs. */
	bytes = 0;
	pos = buf;
	while ((c = getc(stream)) != -1) {
		if (bytes + 1 >= SSIZE_MAX) {
			return 0;
		}
		bytes++;
		if (bytes >= *n - 1) {
			buf = realloc(*lineptr, *n + 128);
			if (buf == NULL) {
				/* ENOMEM */
				return 0;
			}
			*n += 128;
			pos = buf + bytes - 1;
			*lineptr = buf;
		}
		
		*pos++ = (char) c;
		if (c == delimiter) {
			break;
		}
	}
	
	if (ferror(stream) || (feof(stream) && (bytes == 0))) {
		/* EOF, or an error from getc(). */
		return 0;
	}
	
	*pos = '\0';
	return bytes;
}

//Copies source over to the destination pointer.
//TODO: Move this to a better time
void copyString(const char *source, char **destination) {
	*destination = malloc(strlen(source) + 1);
	strcpy(*destination, source);
}

int getFileSize(char *fileName) {
	FILE *file;
	file = fopen(fileName, "r");
	if (!file) return 0;
	fseek(file, 0L, SEEK_END);
	int size = (int)ftell(file);
	fclose(file);
	return size;
}
