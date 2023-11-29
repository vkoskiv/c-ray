//
//  fileio.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../includes.h"
#include <stdbool.h>
#include <stddef.h>
#include "dyn_array.h"

struct file_cache;

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

typedef byte file_bytes;
dyn_array_def(file_bytes);
typedef struct file_bytes_arr file_data;

enum fileType guess_file_type(const char *path);
char *human_file_size(unsigned long bytes, char *stat_buf);
file_data file_load(const char *filePath);
void file_free(file_data *file);
// This is a more robust file writing function, that will seek alternate directories
// if the specified one wasn't writeable.
void write_file(file_data file, const char *path);
bool is_valid_file(char *path);
char *get_file_name(const char *input);
char *get_file_path(const char *input);
// Await for input on stdin for up to 2 seconds. If nothing shows up, return empty file_data
file_data read_stdin(void);
size_t get_file_size(const char *fileName);
