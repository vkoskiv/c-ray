//
//  filecache.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021-2023 Valtteri Koskivuori. All rights reserved.
//

#include "filecache.h"
#include <stdlib.h>
#include "../vendored/cJSON.h"
#include "base64.h"
#include "string.h"
#include <string.h>
#include "logging.h"

bool cache_contains(const struct file_cache *cache, const char *path) {
	for (size_t i = 0; i < cache->files.count; ++i) {
		if (stringEquals(path, cache->files.items[i].path)) {
			return true;
		}
	}
	return false;
}

static void file_elem_free(struct file *f) {
	if (f->path) free(f->path);
	if (f->data.items) {
		// File cache data is received over the wire, so it's always malloc'd
		// normally, files would use file_free(), which munmap()s, if appropriate.
		free(f->data.items);
		f->data.items = NULL;
		f->data.count = 0;
		f->data.capacity = 0;
	}
}

struct file_cache *cache_create(void) {
	struct file_cache *cache = calloc(1, sizeof(*cache));
	cache->files.elem_free = file_elem_free;
	return cache;
}

void cache_store(struct file_cache *cache, const char *path, const void *data, size_t length) {
	if (cache_contains(cache, path)) {
		logr(debug, "File %s already cached, skipping.\n", path);
		return;
	}
	struct file file;
	file.path = stringCopy(path);
	file.data = (file_data){
		.items = malloc(length + 1),
		.count = length,
		.capacity = length,
	};
	memcpy(file.data.items, data, length + 1);
	file_arr_add(&cache->files, file);
	logr(debug, "Cached file %s\n", path);
}

file_data cache_load(const struct file_cache *cache, const char *path) {
	for (size_t i = 0; i < cache->files.count; ++i) {
		if (stringEquals(path, cache->files.items[i].path)) {
			logr(debug, "Retrieving file %s\n", path);
			return cache->files.items[i].data;
		}
	}
	logr(debug, "File %s not found in cache\n", path);
	return (file_data){ 0 };
}

char *cache_encode(const struct file_cache *cache) {
	cJSON *fileCache = cJSON_CreateArray();
	for (size_t i = 0; i < cache->files.count; ++i) {
		cJSON *record = cJSON_CreateObject();
		char *encoded = b64encode(cache->files.items[i].data.items, cache->files.items[i].data.count);
		cJSON_AddStringToObject(record, "path", cache->files.items[i].path);
		cJSON_AddStringToObject(record, "data", encoded);
		free(encoded);
		cJSON_AddItemToArray(fileCache, record);
	}
	char *final = cJSON_PrintUnformatted(fileCache);
	cJSON_Delete(fileCache);
	return final;
}

//FIXME: Are we sure this can't have an error to be returned?
struct file_cache *cache_decode(const char *data) {
	cJSON *receivedCache = cJSON_Parse(data);
	const cJSON *record = NULL;
	struct file_cache *cache = cache_create();
	cJSON_ArrayForEach(record, receivedCache) {
		cJSON *path = cJSON_GetObjectItem(record, "path");
		cJSON *data = cJSON_GetObjectItem(record, "data");
		size_t datalen = 0;
		void *decoded = b64decode(data->valuestring, strlen(data->valuestring), &datalen);
		cache_store(cache, path->valuestring, decoded, datalen);
		free(decoded);
	}
	cJSON_Delete(receivedCache);
	return cache;
}

void cache_destroy(struct file_cache *cache) {
	file_arr_free(&cache->files);
	logr(debug, "Destroyed cache\n");
}
