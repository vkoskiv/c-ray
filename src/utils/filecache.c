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

void cache_store(struct file_cache *cache, const char *path, const void *data, size_t length) {
	if (cache_contains(cache, path)) {
		logr(debug, "File %s already cached, skipping.\n", path);
		return;
	}
	struct file file;
	file.path = stringCopy(path);
	file.data = malloc(length);
	file.size = length;
	memcpy(file.data, data, length);
	file_arr_add(&cache->files, file);
	logr(debug, "Cached file %s\n", path);
}

void *cache_load(const struct file_cache *cache, const char *path, size_t *length) {
	for (size_t i = 0; i < cache->files.count; ++i) {
		if (stringEquals(path, cache->files.items[i].path)) {
			if (length) *length = cache->files.items[i].size;
			char *ret = malloc(cache->files.items[i].size + 1);
			memcpy(ret, cache->files.items[i].data, cache->files.items[i].size);
			ret[cache->files.items[i].size] = 0;
			logr(debug, "Retrieving file %s\n", path);
			return ret;
		}
	}
	logr(debug, "File %s not found in cache\n", path);
	return NULL;
}

char *cache_encode(const struct file_cache *cache) {
	cJSON *fileCache = cJSON_CreateArray();
	for (size_t i = 0; i < cache->files.count; ++i) {
		cJSON *record = cJSON_CreateObject();
		char *encoded = b64encode(cache->files.items[i].data, cache->files.items[i].size);
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
	for (size_t i = 0; i < cache->files.count; ++i) {
		if (cache->files.items[i].data) free(cache->files.items[i].data);
		if (cache->files.items[i].path) free(cache->files.items[i].path);
	}
	file_arr_free(&cache->files);
	logr(debug, "Destroyed cache\n");
}
