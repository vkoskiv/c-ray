//
//  filecache.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "filecache.h"
#include <stdlib.h>
#include "../libraries/cJSON.h"
#include "base64.h"
#include "string.h"
#include <string.h>
#include "logging.h"

struct file {
	char *path;
	size_t size;
	void *data;
};

size_t fileCount = 0;
struct file *cachedFiles = NULL;

bool cacheContains(const char *path) {
	for (size_t i = 0; i < fileCount; ++i) {
		if (stringEquals(path, cachedFiles[i].path)) {
			return true;
		}
	}
	return false;
}

void cacheFile(const char *path, const void *data, size_t length) {
	if (cacheContains(path)) {
		logr(debug, "File %s already cached, skipping.\n", path);
		return;
	}
	cachedFiles = realloc(cachedFiles, ++fileCount * sizeof(*cachedFiles));
	struct file file;
	file.path = stringCopy(path);
	file.data = malloc(length);
	file.size = length;
	memcpy(file.data, data, length);
	cachedFiles[fileCount - 1] = file;
	logr(debug, "Cached file %s\n", path);
}

void *loadFromCache(const char *path, size_t *length) {
	for (size_t i = 0; i < fileCount; ++i) {
		if (stringEquals(path, cachedFiles[i].path)) {
			if (length) *length = cachedFiles[i].size;
			char *ret = malloc(cachedFiles[i].size);
			memcpy(ret, cachedFiles[i].data, cachedFiles[i].size);
			logr(debug, "Retrieving file %s\n", path);
			return ret;
		}
	}
	logr(debug, "File %s not found in cache\n", path);
	return NULL;
}

char *encodeFileCache(void) {
	cJSON *fileCache = cJSON_CreateArray();
	for (size_t i = 0; i < fileCount; ++i) {
		cJSON *record = cJSON_CreateObject();
		char *encoded = b64encode(cachedFiles[i].data, cachedFiles[i].size);
		cJSON_AddStringToObject(record, "path", cachedFiles[i].path);
		cJSON_AddStringToObject(record, "data", encoded);
		free(encoded);
		cJSON_AddItemToArray(fileCache, record);
	}
	char *final = cJSON_PrintUnformatted(fileCache);
	cJSON_Delete(fileCache);
	return final;
}

void decodeFileCache(const char *data) {
	cJSON *receivedCache = cJSON_Parse(data);
	const cJSON *record = NULL;
	cJSON_ArrayForEach(record, receivedCache) {
		cJSON *path = cJSON_GetObjectItem(record, "path");
		cJSON *data = cJSON_GetObjectItem(record, "data");
		size_t datalen = 0;
		void *decoded = b64decode(data->valuestring, strlen(data->valuestring), &datalen);
		cacheFile(path->valuestring, decoded, datalen);
		free(decoded);
	}
	cJSON_Delete(receivedCache);
}

void destroyFileCache() {
	for (size_t i = 0; i < fileCount; ++i) {
		if (cachedFiles[i].data) free(cachedFiles[i].data);
		if (cachedFiles[i].path) free(cachedFiles[i].path);
	}
	free(cachedFiles);
	cachedFiles = NULL;
	fileCount = 0;
	logr(debug, "Destroyed cache\n");
}
