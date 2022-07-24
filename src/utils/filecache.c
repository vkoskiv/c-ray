//
//  filecache.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "filecache.h"
#include <stdlib.h>
#include "../vendored/cJSON.h"
#include "base64.h"
#include "string.h"
#include <string.h>
#include "logging.h"

bool cache_contains(const struct file_cache *cache, const char *path) {
	for (size_t i = 0; i < cache->file_count; ++i) {
		if (stringEquals(path, cache->files[i].path)) {
			return true;
		}
	}
	return false;
}

void cache_store(struct file_cache *cache, const char *path, const void *data, size_t length) {
	if (cache_contains(cache, path)) {
		logr(debug, "File %s already cached, skipping.\n", path);
		return;
	}
	cache->files = realloc(cache->files, ++cache->file_count * sizeof(*cache->files));
	struct file file;
	file.path = stringCopy(path);
	file.data = malloc(length);
	file.size = length;
	memcpy(file.data, data, length);
	cache->files[cache->file_count - 1] = file;
	logr(debug, "Cached file %s\n", path);
}

void *cache_load(const struct file_cache *cache, const char *path, size_t *length) {
	for (size_t i = 0; i < cache->file_count; ++i) {
		if (stringEquals(path, cache->files[i].path)) {
			if (length) *length = cache->files[i].size;
			char *ret = malloc(cache->files[i].size + 1);
			memcpy(ret, cache->files[i].data, cache->files[i].size);
			ret[cache->files[i].size] = 0;
			logr(debug, "Retrieving file %s\n", path);
			return ret;
		}
	}
	logr(debug, "File %s not found in cache\n", path);
	return NULL;
}

char *cache_encode(const struct file_cache *cache) {
	cJSON *fileCache = cJSON_CreateArray();
	for (size_t i = 0; i < cache->file_count; ++i) {
		cJSON *record = cJSON_CreateObject();
		char *encoded = b64encode(cache->files[i].data, cache->files[i].size);
		cJSON_AddStringToObject(record, "path", cache->files[i].path);
		cJSON_AddStringToObject(record, "data", encoded);
		free(encoded);
		cJSON_AddItemToArray(fileCache, record);
	}
	char *final = cJSON_PrintUnformatted(fileCache);
	cJSON_Delete(fileCache);
	return final;
}

//FIXME: Are we sure this can't have an error to be returned?
void cache_decode(struct file_cache *cache, const char *data) {
	cJSON *receivedCache = cJSON_Parse(data);
	const cJSON *record = NULL;
	cJSON_ArrayForEach(record, receivedCache) {
		cJSON *path = cJSON_GetObjectItem(record, "path");
		cJSON *data = cJSON_GetObjectItem(record, "data");
		size_t datalen = 0;
		void *decoded = b64decode(data->valuestring, strlen(data->valuestring), &datalen);
		cache_store(cache, path->valuestring, decoded, datalen);
		free(decoded);
	}
	cJSON_Delete(receivedCache);
}

void cache_destroy(struct file_cache *cache) {
	for (size_t i = 0; i < cache->file_count; ++i) {
		if (cache->files[i].data) free(cache->files[i].data);
		if (cache->files[i].path) free(cache->files[i].path);
	}
	free(cache->files);
	cache->files = NULL;
	cache->file_count = 0;
	logr(debug, "Destroyed cache\n");
}
