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

enum fileType guess_file_type(const char *filePath);
char *human_file_size(unsigned long bytes);
char *load_file(const char *filePath, size_t *bytes, struct file_cache *cache);
// This is a more robust file writing function, that will seek alternate directories
// if the specified one wasn't writeable.
void write_file(const unsigned char *buf, size_t bufsize, const char *filePath);
bool is_valid_file(char *path, struct file_cache *cache);
char *get_file_name(const char *input);
char *get_file_path(const char *input);
/// Await for input on stdin for up to 2 seconds. If nothing shows up, return NULL
/// @param bytes Bytes read, if successful.
char *read_stdin(size_t *bytes);
size_t get_file_size(const char *fileName);
