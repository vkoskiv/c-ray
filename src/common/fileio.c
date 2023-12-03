//
//  fileio.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "fileio.h"
#include "logging.h"
#include "assert.h"
#ifndef WINDOWS
#include <sys/mman.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "string.h"
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "textbuffer.h"

static char *getFileExtension(const char *fileName) {
	if (!fileName) return NULL;
	char buf[LINEBUFFER_MAXSIZE];
	lineBuffer line = { .buf = buf };
	fillLineBuffer(&line, fileName, '.');
	if (line.amountOf.tokens != 2) {
		return NULL;
	}
	char *extension = stringCopy(lastToken(&line));
	return extension;
}

enum fileType match_file_type(const char *ext) {
	if (!ext) return unknown;
	if (stringEquals(ext, "bmp"))
		return bmp;
	if (stringEquals(ext, "png"))
		return png;
	if (stringEquals(ext, "hdr"))
		return hdr;
	if (stringEquals(ext, "obj"))
		return obj;
	if (stringEquals(ext, "mtl"))
		return mtl;
	if (stringEquals(ext, "jpg") || stringEquals(ext, "jpeg"))
		return jpg;
	if (stringEquals(ext, "tiff"))
		return tiff;
	if (stringEquals(ext, "qoi"))
		return qoi;
	if (stringEquals(ext, "gltf"))
		return gltf;
	if (stringEquals(ext, "glb"))
		return glb;
	return unknown;
}

enum fileType guess_file_type(const char *filePath) {
	if (!filePath) return unknown;
	char *fileName = get_file_name(filePath);
	char *extension = getFileExtension(fileName);
	char *lower = stringToLower(extension);
	free(extension);
	extension = lower;
	free(fileName);
	enum fileType type = match_file_type(extension);
	free(extension);
	return type;
}

size_t get_file_size(const char *path) {
#ifndef WINDOWS
	struct stat path_stat = { 0 };
	if (stat(path, &path_stat) < 0) {
		logr(warning, "Couldn't stat '%.*s': %s\n", (int)strlen(path), path, strerror(errno));
		return 0;
	}
	return path_stat.st_size;
#else
	FILE *file = fopen(path, "rb");
	if (!file) {
		logr(warning, "Can't access '%.*s': %s\n", (int)strlen(path), path, strerror(errno));
		return 0;
	}
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fclose(file);
	return size;
#endif
}

#ifdef WINDOWS
typedef size_t off_t;
#endif

file_data file_load(const char *file_path) {
	off_t size = get_file_size(file_path);
	if (size == 0) {
		return (file_data){ 0 };
	}
#ifndef WINDOWS
	int f = open(file_path, 0);
	void *data = mmap(NULL, size, PROT_READ, MAP_SHARED, f, 0);
	if (data == MAP_FAILED) {
		logr(warning, "Couldn't mmap '%.*s': %s\n", (int)strlen(file_path), file_path, strerror(errno));
		return (file_data){ 0 };
	}
	madvise(data, size, MADV_SEQUENTIAL);
	file_data file = (file_data){ .items = data, .count = size, .capacity = size };
	return file;
#else
	FILE *file = fopen(file_path, "rb");
	file_bytes *buf = malloc(size + 1 * sizeof(char));
	size_t readBytes = fread(buf, sizeof(char), size, file);
	ASSERT(readBytes == size);
	if (ferror(file) != 0) {
		logr(warning, "Error reading file\n");
	} else {
		buf[size] = '\0';
	}
	fclose(file);
	file_data filedata = (file_data){ .items = buf, .count = readBytes, .capacity = readBytes };
	return filedata;
#endif
}

void file_free(file_data *file) {
	if (!file || !file->items) return;
#ifndef WINDOWS
	munmap(file->items, file->count);
#else
	free(file->items);
#endif
	file->items = NULL;
	file->capacity = 0;
	file->count = 0;
}

void write_file(file_data data, const char *filePath) {
	FILE *file = fopen(filePath, "wb" );
	char *backupPath = NULL;
	if(!file) {
		char *name = get_file_name(filePath);
		backupPath = stringConcat("./", name);
		free(name);
		file = fopen(backupPath, "wb");
		if (file) {
			char *path = get_file_path(filePath);
			logr(warning, "The specified output directory \"%s\" was not writeable, dumping the file in CWD instead.\n", path);
			free(path);
		} else {
			logr(warning, "Neither the specified output directory nor the current working directory were writeable. Image can't be saved. Fix your permissions!");
			return;
		}
	}
	logr(info, "Saving result in %s\'%s\'%s\n", KGRN, backupPath ? backupPath : filePath, KNRM);
	fwrite(data.items, 1, data.count, file);
	fclose(file);
	
	//We determine the file size after saving, because the lodePNG library doesn't have a way to tell the compressed file size
	//This will work for all image formats
	unsigned long bytes = get_file_size(backupPath ? backupPath : filePath);
	char buf[64];
	logr(info, "Wrote %s to file.\n", human_file_size(bytes, buf));
}


bool is_valid_file(char *path) {
#ifndef WINDOWS
	struct stat path_stat = { 0 };
	stat(path, &path_stat);
	return S_ISREG(path_stat.st_mode);
#else
	FILE *f = fopen(path, "r");
	if (f) {
		fclose(f);
		return true;
	}
	return false;
#endif
}

void wait_for_stdin(int seconds) {
#ifndef WINDOWS
	fd_set set;
	struct timeval timeout;
	int rv;
	FD_ZERO(&set);
	FD_SET(0, &set);
	timeout.tv_sec = seconds;
	timeout.tv_usec = 1000;
	rv = select(1, &set, NULL, NULL, &timeout);
	if (rv == -1) {
		logr(error, "Error on stdin timeout\n");
	} else if (rv == 0) {
		logr(error, "No input found after %i seconds. Hint: Try `./bin/c-ray input/scene.json`.\n", seconds);
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
//FIXME: Just return a pointer to the first byte of the filename? Why do we do all this
char *get_file_name(const char *input) {
	if (!input) return NULL;
	//FIXME: We're doing two copies here, maybe just rework the algorithm instead.
	char *copy = stringCopy(input);
	char *fn;
	
	/* handle trailing '/' e.g.
	 input == "/home/me/myprogram/" */
	if (copy[(strlen(copy) - 1)] == '/')
		copy[(strlen(copy) - 1)] = '\0';
	
	(fn = strrchr(copy, '/')) ? ++fn : (fn = copy);
	
	char *ret = stringCopy(fn);
	free(copy);
	
	return ret;
}

//For Windows
#define CRAY_PATH_MAX 4096

char *get_file_path(const char *input) {
	char *dir = NULL;
#ifdef WINDOWS
	dir = calloc(_MAX_DIR, sizeof(*dir));
	_splitpath_s(input, NULL, 0, dir, _MAX_DIR, NULL, 0, NULL, 0);
	return dir;
#else
	char *inputCopy = stringCopy(input);
	dir = stringCopy(dirname(inputCopy));
	free(inputCopy);
	char *final = stringConcat(dir, "/");
	free(dir);
	return final;
#endif
}

#define chunksize 65536
//Get scene data from stdin and return a pointer to it
file_data read_stdin(void) {
	wait_for_stdin(2);
	
	char chunk[chunksize];
	
	size_t buf_size = 0;
	unsigned char *buf = NULL;
	int stdin_fd = fileno(stdin);
	int read_bytes = 0;
	while ((read_bytes = read(stdin_fd, &chunk, chunksize)) > 0) {
		unsigned char *old = buf;
		buf = realloc(buf, buf_size + read_bytes + 1);
		if (!buf) {
			logr(error, "Failed to realloc stdin buffer\n");
			free(old);
			return (file_data){ 0 };
		}
		memcpy(buf + buf_size, chunk, read_bytes);
		buf_size += read_bytes;
	}
	
	if (ferror(stdin)) {
		logr(error, "Failed to read from stdin\n");
		free(buf);
		return (file_data){ 0 };
	}
	
	buf[buf_size ] = 0;
	return (file_data){ .items = buf, .count = buf_size - 1, .capacity = buf_size - 1 };
}

char *human_file_size(unsigned long bytes, char *stat_buf) {
	double kilobytes, megabytes, gigabytes, terabytes, petabytes, exabytes, zettabytes, yottabytes; // <- Futureproofing?!
	kilobytes  = bytes      / 1000.0;
	megabytes  = kilobytes  / 1000.0;
	gigabytes  = megabytes  / 1000.0;
	terabytes  = gigabytes  / 1000.0;
	petabytes  = terabytes  / 1000.0;
	exabytes   = petabytes  / 1000.0;
	zettabytes = exabytes   / 1000.0;
	yottabytes = zettabytes / 1000.0;
	
	// Okay, okay. In reality, this never gets even close to a zettabyte,
	// it'll overflow at around 18 exabytes.
	// I *did* get it to go to yottabytes using __uint128_t, but that's
	// not in C99. Maybe in the future.
	
	char *buf = stat_buf ? stat_buf : calloc(64, sizeof(*buf));
	
	if (zettabytes >= 1000) {
		sprintf(buf, "%.02fYB", yottabytes);
	} else if (exabytes >= 1000) {
		sprintf(buf, "%.02fZB", zettabytes);
	} else if (petabytes >= 1000) {
		sprintf(buf, "%.02fEB", exabytes);
	} else if (terabytes >= 1000) {
		sprintf(buf, "%.02fPB", petabytes);
	} else if (gigabytes >= 1000) {
		sprintf(buf, "%.02fTB", terabytes);
	} else if (megabytes >= 1000) {
		sprintf(buf, "%.02fGB", gigabytes);
	} else if (kilobytes >= 1000) {
		sprintf(buf, "%.02fMB", megabytes);
	} else if (bytes >= 1000) {
		sprintf(buf, "%.02fkB", kilobytes);
	} else {
		sprintf(buf, "%ldB", bytes);
	}
	return buf;
}
