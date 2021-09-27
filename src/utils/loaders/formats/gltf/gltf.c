//
//  gltf.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 26/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "../../../../includes.h"

#include "gltf.h"
#include "../../../../libraries/cJSON.h"
#include "../../../string.h"
#include "../../../base64.h"
#include "../../../../datatypes/vector.h"
#include "../../../logging.h"
#include <string.h>
#include "../../../fileio.h"

enum accessor_type {
	VEC3,
	VEC2,
	SCALAR,
};

//TODO: Max and min, what are those used for even?
struct accessor {
	int buffer_view;
	int componentType; // enum? Not sure what this is
	size_t count;
	enum accessor_type type;
};

struct buffer_view {
	int buffer;
	size_t byte_length;
	size_t byte_offset;
};

char *parse_buffer(const cJSON *data) {
	const cJSON *byteLength = cJSON_GetObjectItem(data, "byteLength");
	if (!cJSON_IsNumber(byteLength)) return NULL;
	const cJSON *uri = cJSON_GetObjectItem(data, "uri");
	if (!cJSON_IsString(uri)) return NULL;
	
	size_t expected_bytes = byteLength->valueint;
	char *uri_string = uri->valuestring;
	
	char *buffer = NULL;
	char *prefix = "data:application/octet-stream;base64,";
	if (stringStartsWith(prefix, uri_string)) {
		// Cool, it's an embedded gltf with b64 data
		char *base64data = uri_string + strlen(prefix);
		size_t encoded_length = strlen(base64data);
		size_t decoded_length = 0;
		buffer = b64decode(base64data, encoded_length, &decoded_length);
		if (decoded_length != expected_bytes) {
			logr(warning, "Invalid buffer while parsing glTF. base64 decoded length of %lu, expected %lu", decoded_length, expected_bytes);
			free(buffer);
			return NULL;
		}
		return buffer;
	} else {
		// Otherwise just try to load the specified file
		if (!isValidFile(uri_string)) {
			logr(warning, "Invalid buffer while parsing glTF. File %s not found.\n", uri_string);
			return NULL;
		}
		size_t loaded_bytes = 0;
		buffer = loadFile(uri_string, &loaded_bytes);
		if (loaded_bytes != expected_bytes) {
			logr(warning, "Invalid buffer while parsing glTF. Loaded file %s length %lu, expected %lu", uri_string, loaded_bytes, expected_bytes);
		}
		return buffer;
	}
	
	return NULL;
}

char **parse_buffers(const cJSON *data, size_t *amount) {
	const cJSON *buffers_object = cJSON_GetObjectItem(data, "buffers");
	char **buffers = NULL;
	size_t buffer_amount = 0;
	if (cJSON_IsArray(buffers_object)) {
		buffer_amount = cJSON_GetArraySize(buffers_object);
		buffers = calloc(buffer_amount, sizeof(*buffers));
		for (size_t i = 0; i < buffer_amount; ++i) {
			buffers[i] = parse_buffer(cJSON_GetArrayItem(buffers_object, (int)i));
		}
	}
	if (amount) *amount = buffer_amount;
	return buffers;
}

struct buffer_view *parse_buffer_views(const cJSON *data, size_t *amount) {
	size_t buffer_view_amount = 0;
	if (!cJSON_IsArray(data)) return NULL;
	
	buffer_view_amount = cJSON_GetArraySize(data);
	struct buffer_view *views = calloc(buffer_view_amount, sizeof(*views));
	for (size_t i = 0; i < buffer_view_amount; ++i) {
		cJSON *element = cJSON_GetArrayItem(data, (int)i);
		// FIXME: Validate
		views[i].buffer = cJSON_GetObjectItem(element, "buffer")->valueint;
		views[i].byte_length = cJSON_GetObjectItem(element, "byteLength")->valueint;
		views[i].byte_offset = cJSON_GetObjectItem(element, "byteOffset")->valueint;
	}
	
	if (amount) *amount = buffer_view_amount;
	return views;
}

struct accessor *parse_accessors(const cJSON *data, size_t *amount) {
	// TODO
	return NULL;
}

static int parse_glTF(struct renderer *r, char *input, bool only_assets) {
	const cJSON *data = cJSON_Parse(input);
	
	size_t buffers_count = 0;
	char **buffers = parse_buffers(cJSON_GetObjectItem(data, "buffers"), &buffers_count);
	
	size_t buffer_views_count = 0;
	struct buffer_view *views = parse_buffer_views(data, &buffer_views_count);
	
	//TODO: Validate that buffer views are valid, perhaps?
	
	
	
	return 0;
}

struct mesh *parseglTF(const char *filePath, size_t *meshCount) {
	//TODO
	(void)filePath;
	if (meshCount) *meshCount = 0;
	return NULL;
}
