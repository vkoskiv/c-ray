//
//  filecache.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <sys/types.h>
#include <stdbool.h>

struct file {
	char *path;
	size_t size;
	void *data;
};

struct file_cache {
	size_t file_count;
	struct file *files;
};

bool cache_contains(const struct file_cache *cache, const char *path);

void cache_store(struct file_cache *cache, const char *path, const void *data, size_t length);

void *cache_load(const struct file_cache *cache, const char *path, size_t *length);

char *cache_encode(const struct file_cache *cache);

void cache_decode(struct file_cache *cache, const char *data);

void cache_destroy(struct file_cache *cache);
