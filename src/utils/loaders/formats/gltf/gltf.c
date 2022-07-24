//
//  gltf.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 26/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "../../../../includes.h"

#include "gltf.h"
#include "../../../../vendored/cJSON.h"
#include "../../../string.h"
#include "../../../base64.h"
#include "../../../../datatypes/vector.h"
#include "../../../logging.h"
#include <string.h>
#include "../../../fileio.h"
#include "../../../../datatypes/image/texture.h"
#include "../../textureloader.h"

enum accessor_type {
	UNKNOWN,
	VEC2,
	VEC3,
	SCALAR,
};

//TODO: Max and min, what are those used for even? Integrity checking?
struct accessor {
	size_t buffer_view_idx;
	size_t byte_offset;
	enum accessor_type type;
	size_t count;
};

struct buffer_view {
	size_t buffer_idx;
	size_t byte_length;
	size_t byte_offset;
	size_t byte_stride;
};

size_t get_int_or_zero(const cJSON *object, const char *key) {
	return cJSON_HasObjectItem(object, key) ? cJSON_GetObjectItem(object, key)->valueint : 0;
}

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
			logr(warning, "Invalid buffer while parsing glTF. base64 decoded length of %lu, expected %lu\n", decoded_length, expected_bytes);
			free(buffer);
			return NULL;
		}
		return buffer;
	} else {
		// Otherwise just try to load the specified file
		if (!isValidFile(uri_string, NULL)) { //FIXME cache
			logr(warning, "Invalid buffer while parsing glTF. File %s not found.\n", uri_string);
			return NULL;
		}
		size_t loaded_bytes = 0;
		buffer = loadFile(uri_string, &loaded_bytes, NULL); //FIXME cache
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
		const cJSON *element = cJSON_GetArrayItem(data, (int)i);
		// FIXME: Validate
		views[i].buffer_idx = get_int_or_zero(element, "buffer");
		views[i].byte_length = get_int_or_zero(element, "byteLength");
		views[i].byte_offset = get_int_or_zero(element, "byteOffset");
		views[i].byte_stride = get_int_or_zero(element, "byteStride");
	}
	
	if (amount) *amount = buffer_view_amount;
	return views;
}

enum accessor_type accessor_type_for_string(const char *str) {
	if (stringEquals(str, "VEC2"))
		return VEC2;
	if (stringEquals(str, "VEC3"))
		return VEC3;
	if (stringEquals(str, "SCALAR"))
		return SCALAR;
	return UNKNOWN;
}

struct accessor *parse_accessors(const cJSON *data, size_t *amount) {
	size_t accessor_amount = 0;
	if (!cJSON_IsArray(data)) return NULL;
	
	accessor_amount = cJSON_GetArraySize(data);
	struct accessor *accessors = calloc(accessor_amount, sizeof(*accessors));
	for (size_t i = 0; i < accessor_amount; ++i) {
		// FIXME: Validate
		const cJSON *element = cJSON_GetArrayItem(data, (int)i);
		accessors[i].buffer_view_idx = cJSON_GetObjectItem(element, "bufferView")->valueint;
		accessors[i].byte_offset = cJSON_HasObjectItem(element, "byteOffset") ? cJSON_GetObjectItem(element, "byteOffset")->valueint : 0;
		accessors[i].type = accessor_type_for_string(cJSON_GetObjectItem(element, "type")->valuestring);
		accessors[i].count = cJSON_GetObjectItem(element, "count")->valueint;
	}
	
	if (amount) *amount = accessor_amount;
	return accessors;
}

//TODO: Figure out what the stride means and how it works
// the glTF doc was quite vague about it. For now, we ignore it.
char *bytes_from_buffer_view(const struct buffer_view *view, char **buffers) {
	char *data = malloc(view->byte_length);
	memcpy(data, buffers[view->buffer_idx] + view->byte_offset, view->byte_length);
	return data;
}

struct texture *parse_images(const cJSON *data, size_t *amount, const struct buffer_view *views, char **buffers) {
	if (!cJSON_IsArray(data)) return NULL;
	
	size_t image_amount = 0;
	image_amount = cJSON_GetArraySize(data);
	struct texture *images = calloc(image_amount, sizeof(*images));
	for (size_t i = 0; i < image_amount; ++i) {
		const cJSON *element = cJSON_GetArrayItem(data, (int)i);
		if (cJSON_HasObjectItem(element, "uri")) {
			const cJSON *uri = cJSON_GetObjectItem(element, "uri");
			if (!cJSON_IsString(uri)) break;
			char *uri_string = uri->valuestring;
			images[i] = *load_texture(uri_string, NULL, NULL); //FIXME cache
		} else {
			const cJSON *buffer_view = cJSON_GetObjectItem(element, "bufferView");
			const cJSON *mime_type = cJSON_GetObjectItem(element, "mimeType");
			if (!cJSON_IsNumber(buffer_view) || !cJSON_IsString(mime_type)) break;
			size_t buffer_view_idx = buffer_view->valueint;
			char *bytes = bytes_from_buffer_view(&views[buffer_view_idx], buffers);
			images[i] = *load_texture_from_buffer((unsigned char *)data, (unsigned int)views[buffer_view_idx].byte_length, NULL);
			free(bytes);
		}
	}
	if (amount) *amount = image_amount;
	return images;
}

struct mesh *parse_glb_meshes(const char *data, size_t *meshCount) {
	(void )data;
	(void )meshCount;
	ASSERT_NOT_REACHED();
	return NULL;
}

struct mesh *parse_glTF_meshes(const char *filePath, size_t *meshCount) {
	size_t file_bytes = 0;
	char *contents = loadFile(filePath, &file_bytes, NULL); //FIXME cache
	if (stringStartsWith("glTF", contents)) return parse_glb_meshes(contents, meshCount);
	const cJSON *data = cJSON_Parse(contents);
	
	const cJSON *asset = cJSON_GetObjectItem(data, "asset");
	if (asset) {
		const cJSON *generator = cJSON_GetObjectItem(asset, "generator");
		const cJSON *version = cJSON_GetObjectItem(asset, "version");
		if (cJSON_IsString(generator) && cJSON_IsString(version)) {
			logr(debug, "Parsing glTF file \"%s\" Generator: \"%s\", glTF version %s\n", filePath, generator->valuestring, version->valuestring);
		}
	}
	
	size_t buffers_count = 0;
	char **buffers = parse_buffers(data, &buffers_count);
	
	//TODO: Validate that buffer views are valid, perhaps?
	size_t buffer_views_count = 0;
	struct buffer_view *buffer_views = parse_buffer_views(cJSON_GetObjectItem(data, "bufferViews"), &buffer_views_count);
	
	size_t accessors_count = 0;
	struct accessor *accessors = parse_accessors(cJSON_GetObjectItem(data, "accessors"), &accessors_count);
	(void)accessors; //TODO
	
	size_t texture_count = 0;
	struct texture *textures = parse_images(cJSON_GetObjectItem(data, "images"), &texture_count, buffer_views, buffers);
	(void)textures; //TODO
	
	if (meshCount) *meshCount = 0;
	return NULL;
}
