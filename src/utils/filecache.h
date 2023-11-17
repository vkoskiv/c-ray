//
//  filecache.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <sys/types.h>
#include <stdbool.h>
#include "dyn_array.h"
#include "fileio.h"

struct file {
	char *path;
	file_data data;
};

typedef struct file file;
dyn_array_def(file);

struct file_cache {
	struct file_arr files;
};

struct file_cache *cache_create(void);
bool cache_contains(const struct file_cache *cache, const char *path);
void cache_store(struct file_cache *cache, const char *path, const void *data, size_t length);
file_data cache_load(const struct file_cache *cache, const char *path);
char *cache_encode(const struct file_cache *cache);
struct file_cache *cache_decode(const char *data);
void cache_destroy(struct file_cache *cache);
