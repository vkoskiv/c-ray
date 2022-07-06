//
//  fileio.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "filecache.h"

enum fileType {
	unknown,
	bmp,
	png,
	hdr,
	obj,
	mtl,
	jpg,
	tiff,
	qoi,
	gltf,
	glb,
};

enum fileType guessFileType(const char *filePath);

/// Returns a string containing `bytes` converted into a more human readable format.
/// @param bytes How many bytes you have
/// @return A human readable file size string.
char *humanFileSize(unsigned long bytes);

/// Load a file from a given path
/// @param fileName Path to file
/// @param bytes Will be set to amount of bytes read, if provided.
/// @param cache Optional cache, will be queried first before loading from disk.
char *loadFile(const char *filePath, size_t *bytes, struct file_cache *cache);

// This is a more robust file writing function, that will seek alternate directories
// if the specified one wasn't writeable.
void writeFile(const unsigned char *buf, size_t bufsize, const char *filePath);

/// Returns true if the file at the given path exists and is readable.
/// @param path Path to check
bool isValidFile(char *path, struct file_cache *cache);

/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(const char *input);

/// Extract the path from a given full path, excluding the filename
/// @param input Full path
char *getFilePath(const char *input);

/// Await for input on stdin for up to 2 seconds. If nothing shows up, return NULL
/// @param bytes Bytes read, if successful.
char *readStdin(size_t *bytes);

/// Check the size of a given file in bytes.
/// @param fileName File to check
size_t getFileSize(const char *fileName);
