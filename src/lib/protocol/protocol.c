//
//  protocol.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "protocol.h"
#include <stdio.h>

#ifndef WINDOWS

#include <string.h>
#include <stdlib.h>

#include "../../common/logging.h"
#include "../../common/vector.h"
#include "../../common/texture.h"
#include "../../common/transforms.h"
#include "../../common/quaternion.h"
#include "../../common/hashtable.h"
#include "../../common/string.h"
#include "../../common/gitsha1.h"
#include "../../common/networking.h"
#include "../../common/base64.h"
#include "../../common/timer.h"
#include "../../common/node_parse.h"
#include "../renderer/renderer.h"
#include "../renderer/instance.h"
#include "../datatypes/tile.h"
#include "../datatypes/scene.h"
#include "assert.h"

// Consumes given json, no need to free it after.
bool sendJSON(int socket, cJSON *json, size_t *progress) {
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	bool ret = chunkedSend(socket, jsonText, progress);
	free(jsonText);
	return ret;
}

cJSON *readJSON(int socket) {
	char *recvBuf = NULL;
	size_t length = 0;
	struct timeval timer;
	timer_start(&timer);
	if (chunkedReceive(socket, &recvBuf, &length) == 0) {
		return NULL;
	}
	
	cJSON *received = cJSON_Parse(recvBuf);
	free(recvBuf);
	return received;
}

cJSON *errorResponse(const char *error) {
	cJSON *errorMsg = cJSON_CreateObject();
	cJSON_AddStringToObject(errorMsg, "error", error);
	return errorMsg;
}

cJSON *goodbye() {
	cJSON *goodbye = cJSON_CreateObject();
	cJSON_AddStringToObject(goodbye, "action", "goodbye");
	return goodbye;
}

cJSON *newAction(const char *action) {
	if (!action) return NULL;
	cJSON *actionJson = cJSON_CreateObject();
	cJSON_AddStringToObject(actionJson, "action", action);
	return actionJson;
}

cJSON *encodeTile(const struct render_tile *tile) {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "width", tile->width);
	cJSON_AddNumberToObject(json, "height", tile->height);
	cJSON_AddNumberToObject(json, "beginX", tile->begin.x);
	cJSON_AddNumberToObject(json, "beginY", tile->begin.y);
	cJSON_AddNumberToObject(json, "endX", tile->end.x);
	cJSON_AddNumberToObject(json, "endY", tile->end.y);
	cJSON_AddNumberToObject(json, "state", tile->state);
	cJSON_AddNumberToObject(json, "index", tile->index);
	cJSON_AddNumberToObject(json, "completed_samples", tile->completed_samples);
	cJSON_AddNumberToObject(json, "total_samples", tile->total_samples);
	return json;
}

struct render_tile decodeTile(const cJSON *json) {
	struct render_tile tile = {0};
	tile.width = cJSON_GetObjectItem(json, "width")->valueint;
	tile.height = cJSON_GetObjectItem(json, "height")->valueint;
	tile.begin.x = cJSON_GetObjectItem(json, "beginX")->valueint;
	tile.begin.y = cJSON_GetObjectItem(json, "beginY")->valueint;
	tile.end.x = cJSON_GetObjectItem(json, "endX")->valueint;
	tile.end.y = cJSON_GetObjectItem(json, "endY")->valueint;
	tile.state = cJSON_GetObjectItem(json, "state")->valueint;
	tile.index = cJSON_GetObjectItem(json, "index")->valueint;
	tile.completed_samples = cJSON_GetObjectItem(json, "completed_samples")->valueint;
	tile.total_samples = cJSON_GetObjectItem(json, "total_samples")->valueint;
	return tile;
}

cJSON *serialize_texture(const struct texture *t) {
	if (!t) return NULL;
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "width", t->width);
	cJSON_AddNumberToObject(json, "height", t->height);
	cJSON_AddNumberToObject(json, "channels", t->channels);
	size_t primSize = t->precision == char_p ? sizeof(char) : sizeof(float);
	size_t bytes = t->width * t->height * t->channels * primSize;
	char *encoded = b64encode(t->data.byte_p, bytes);
	cJSON_AddStringToObject(json, "data", encoded);
	cJSON_AddBoolToObject(json, "isFloatPrecision", t->precision == float_p);
	free(encoded);
	return json;
}

struct texture *deserialize_texture(const cJSON *json) {
	if (!json) return NULL;
	struct texture *tex = calloc(1, sizeof(*tex));
	tex->colorspace = linear;
	char *data = cJSON_GetStringValue(cJSON_GetObjectItem(json, "data"));
	tex->data.byte_p = b64decode(data, strlen(data), NULL);
	tex->width = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "width"));
	tex->height = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "height"));
	tex->channels = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "channels"));
	tex->precision = cJSON_IsTrue(cJSON_GetObjectItem(json, "isFloatPrecision")) ? float_p : char_p;
	return tex;
}

int matchCommand(const struct command *cmdlist, size_t commandCount, const char *cmd) {
	for (size_t i = 0; i < commandCount; ++i) {
		if (stringEquals(cmdlist[i].name, cmd)) return cmdlist[i].id;
	}
	return -1;
}

bool containsError(const cJSON *json) {
	const cJSON *error = cJSON_GetObjectItem(json, "error");
	if (cJSON_IsString(error)) {
		return true;
	}
	return false;
}

bool containsGoodbye(const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "goodbye")) {
			return true;
		}
	}
	return false;
}

bool containsStats(const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "stats")) {
			return true;
		}
	}
	return false;
}

static cJSON *serialize_coord(const struct coord in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddNumberToObject(out, "x", in.x);
	cJSON_AddNumberToObject(out, "y", in.y);
	return out;
}

static struct coord deserialize_coord(const cJSON *in) {
	struct coord out = { 0 };
	if (!in) return out;
	out.x = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "x"));
	out.y = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "y"));
	return out;
}

static cJSON *serialize_vector(const struct vector in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddNumberToObject(out, "x", in.x);
	cJSON_AddNumberToObject(out, "y", in.y);
	cJSON_AddNumberToObject(out, "z", in.z);
	return out;
}

static struct vector deserialize_vector(const cJSON *in) {
	struct vector out = { 0 };
	if (!in) return out;
	out.x = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "x"));
	out.y = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "y"));
	out.z = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "z"));
	return out;
}

static cJSON *serialize_transform(const struct transform in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "A", cJSON_CreateFloatArray((float *)&in.A, (sizeof(in.A) / sizeof(float)))); // sus
	cJSON_AddItemToObject(out, "Ainv", cJSON_CreateFloatArray((float *)&in.Ainv, (sizeof(in.Ainv) / sizeof(float)))); // sus
	return out;
}

struct transform deserialize_transform(const cJSON *in) {
	struct transform out = { 0 };
	if (!in) return out;
	cJSON *A = cJSON_GetObjectItem(in, "A");
	if (cJSON_IsArray(A)) {
		for (size_t i = 0; i < (sizeof(out.A) / sizeof(float)); ++i) {
			((float *)out.A.mtx)[i] = cJSON_GetNumberValue(cJSON_GetArrayItem(A, i));
		}
	}
	cJSON *Ainv = cJSON_GetObjectItem(in, "Ainv");
	if (cJSON_IsArray(A)) {
		for (size_t i = 0; i < (sizeof(out.Ainv) / sizeof(float)); ++i) {
			((float *)out.Ainv.mtx)[i] = cJSON_GetNumberValue(cJSON_GetArrayItem(Ainv, i));
		}
	}
	return out;
}

static cJSON *serialize_euler_angles(const struct euler_angles in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddNumberToObject(out, "roll", in.roll);
	cJSON_AddNumberToObject(out, "pitch", in.pitch);
	cJSON_AddNumberToObject(out, "yaw", in.yaw);
	return out;
}

static struct euler_angles deserialize_euler_angles(const cJSON *in) {
	struct euler_angles out = { 0 };
	if (!in) return out;
	out.roll = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "roll"));
	out.pitch = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "pitch"));
	out.yaw = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "yaw"));
	return out;
}

// We likely need to convert to network byte order before packing
// TODO: These would benefit much from zlib and not having to b64
static cJSON *serialize_vertex_buffer(const struct vertex_buffer in) {
	cJSON *out = cJSON_CreateObject();

	cJSON_AddNumberToObject(out, "vertex_count", in.vertices.count);
	if (in.vertices.count) {
		char *data = b64encode(in.vertices.items, in.vertices.count * sizeof(*in.vertices.items));
		cJSON_AddStringToObject(out, "vertices", data);
		free(data);
	}

	cJSON_AddNumberToObject(out, "normal_count", in.normals.count);
	if (in.normals.count) {
		char *data = b64encode(in.normals.items, in.normals.count * sizeof(*in.normals.items));
		cJSON_AddStringToObject(out, "normals", data);
		free(data);
	}

	cJSON_AddNumberToObject(out, "texture_coord_count", in.texture_coords.count);
	if (in.texture_coords.count) {
		char *data = b64encode(in.texture_coords.items, in.texture_coords.count * sizeof(*in.texture_coords.items));
		cJSON_AddStringToObject(out, "texture_coords", data);
		free(data);
	}
	return out;
}

static struct vertex_buffer deserialize_vertex_buffer(const cJSON *in) {
	struct vertex_buffer out = { 0 };
	if (!in) return out;

	size_t vertex_count = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "vertex_count"));
	char *v_b64 = cJSON_GetStringValue(cJSON_GetObjectItem(in, "vertices"));
	size_t out_bytes = 0;
	if (v_b64 && vertex_count) {
		struct vector *vertices = b64decode(v_b64, strlen(v_b64), &out_bytes);
		ASSERT(out_bytes == vertex_count * sizeof(struct vector));
		for (size_t i = 0; i < vertex_count; ++i) {
			vector_arr_add(&out.vertices, vertices[i]);
		}
		free(vertices);
	}

	size_t normal_count = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "normal_count"));
	char *n_b64 = cJSON_GetStringValue(cJSON_GetObjectItem(in, "normals"));
	if (n_b64 && normal_count) {
		struct vector *normals = b64decode(n_b64, strlen(n_b64), &out_bytes);
		ASSERT(out_bytes == normal_count * sizeof(struct vector));
		for (size_t i = 0; i < normal_count; ++i) {
			vector_arr_add(&out.normals, normals[i]);
		}
		free(normals);
	}

	size_t texture_coord_count = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "texture_coord_count"));
	char *t_b64 = cJSON_GetStringValue(cJSON_GetObjectItem(in, "texture_coords"));
	if (t_b64 && texture_coord_count) {
		struct coord *texture_coords = b64decode(t_b64, strlen(t_b64), &out_bytes);
		ASSERT(out_bytes == texture_coord_count * sizeof(struct coord));
		for (size_t i = 0; i < texture_coord_count; ++i) {
			coord_arr_add(&out.texture_coords, texture_coords[i]);
		}
		free(texture_coords);
	}

	return out;
}

static cJSON *serialize_faces(const struct poly_arr in) {
	if (!in.count) return NULL;
	cJSON *out = cJSON_CreateObject();
	size_t bytes = in.count * sizeof(*in.items);
	char *encoded = b64encode(in.items, bytes);
	cJSON_AddStringToObject(out, "data", encoded);
	free(encoded);
	cJSON_AddNumberToObject(out, "poly_count", in.count);
	return out;
}

struct poly_arr deserialize_faces(const cJSON *in) {
	struct poly_arr out = { 0 };
	if (!in) return out;
	size_t poly_count = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "poly_count"));
	char *p_b64 = cJSON_GetStringValue(cJSON_GetObjectItem(in, "data"));
	if (p_b64 && poly_count) {
		size_t out_len = 0;
		struct poly *polys = b64decode(p_b64, strlen(p_b64), &out_len);
		ASSERT(out_len == poly_count * sizeof(struct poly));
		for (size_t i = 0; i < poly_count; ++i) {
			poly_arr_add(&out, polys[i]);
		}
		free(polys);
	}
	return out;
}

static cJSON *serialize_mesh(const struct mesh in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "polygons", serialize_faces(in.polygons));
	cJSON_AddNumberToObject(out, "vbuf_idx", in.vbuf_idx);
	// TODO: name
	return out;
}

static struct mesh deserialize_mesh(const cJSON *in) {
	struct mesh out = { 0 };
	if (!in) return out;

	out.polygons = deserialize_faces(cJSON_GetObjectItem(in, "polygons"));
	out.vbuf_idx = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "vbuf_idx"));

	return out;
}

static cJSON *serialize_sphere(const struct sphere in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddNumberToObject(out, "radius", in.radius);
	cJSON_AddNumberToObject(out, "rayOffset", in.rayOffset);
	return out;
}

static sphere deserialize_sphere(const cJSON *in) {
	struct sphere out = { 0 };
	if (!in) return out;
	out.radius = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "radius"));
	out.rayOffset = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "rayOffset"));
	return out;
}

static cJSON *serialize_instance(const struct instance in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "composite", serialize_transform(in.composite));
	cJSON_AddNumberToObject(out, "object_idx", in.object_idx);
	cJSON_AddNumberToObject(out, "bbuf_idx", in.bbuf_idx);
	cJSON_AddBoolToObject(out, "is_mesh", isMesh(&in));
	return out;
}

static struct instance deserialize_instance(const cJSON *in) {
	if (!in) return (struct instance){ 0 };
	size_t object_idx = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "object_idx"));
	bool is_mesh = cJSON_IsTrue(cJSON_GetObjectItem(in, "is_mesh"));

	struct instance out = { 0 };
	if (is_mesh) {
		out = new_mesh_instance(NULL, object_idx, NULL, NULL);
	} else {
		out = new_sphere_instance(NULL, object_idx, NULL, NULL);
	}

	out.composite = deserialize_transform(cJSON_GetObjectItem(in, "composite"));
	out.bbuf_idx = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "bbuf_idx"));

	return out;
}

static cJSON *serialize_camera(const struct camera in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddNumberToObject(out, "FOV", in.FOV);
	cJSON_AddNumberToObject(out, "focal_length", in.focal_length);
	cJSON_AddNumberToObject(out, "focus_distance", in.focus_distance);
	cJSON_AddNumberToObject(out, "fstops", in.fstops);
	cJSON_AddNumberToObject(out, "aperture", in.aperture);
	cJSON_AddNumberToObject(out, "aspect_ratio", in.aspect_ratio);
	cJSON_AddItemToObject(out, "sensor_size", serialize_coord(in.sensor_size));
	cJSON_AddItemToObject(out, "up", serialize_vector(in.up));
	cJSON_AddItemToObject(out, "right", serialize_vector(in.right));
	cJSON_AddItemToObject(out, "look_at", serialize_vector(in.look_at));
	cJSON_AddItemToObject(out, "forward", serialize_vector(in.forward));
	cJSON_AddItemToObject(out, "composite", serialize_transform(in.composite));
	cJSON_AddItemToObject(out, "orientation", serialize_euler_angles(in.orientation));
	cJSON_AddItemToObject(out, "position", serialize_vector(in.position));
	// TODO: bezier path
	cJSON_AddNumberToObject(out, "time", in.time);
	cJSON_AddNumberToObject(out, "width", in.width);
	cJSON_AddNumberToObject(out, "height", in.height);
	return out;
}

// FIXME: We probably don't need the ones we compute anyway when updating camera
static struct camera deserialize_camera(const cJSON *in) {
	struct camera out = { 0 };
	if (!in) return out;
	out.FOV = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "FOV"));
	out.focal_length = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "focal_length"));
	out.focus_distance = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "focus_distance"));
	out.fstops = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "fstops"));
	out.aperture = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "aperture"));
	out.aspect_ratio = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "aspect_ratio"));
	out.sensor_size = deserialize_coord(cJSON_GetObjectItem(in, "sensor_size"));
	out.up = deserialize_vector(cJSON_GetObjectItem(in, "up"));
	out.right = deserialize_vector(cJSON_GetObjectItem(in, "right"));
	out.look_at = deserialize_vector(cJSON_GetObjectItem(in, "look_at"));
	out.forward = deserialize_vector(cJSON_GetObjectItem(in, "forward"));
	out.composite = deserialize_transform(cJSON_GetObjectItem(in, "composite"));
	out.orientation = deserialize_euler_angles(cJSON_GetObjectItem(in, "orientation"));
	out.position = deserialize_vector(cJSON_GetObjectItem(in, "position"));
	out.time = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "time"));
	out.width = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "width"));
	out.height = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "height"));

	return out;
}

static cJSON *serialize_color(const struct cr_color in) {
	cJSON *out = cJSON_CreateArray();
	cJSON_AddItemToArray(out, cJSON_CreateNumber(in.r));
	cJSON_AddItemToArray(out, cJSON_CreateNumber(in.g));
	cJSON_AddItemToArray(out, cJSON_CreateNumber(in.b));
	cJSON_AddItemToArray(out, cJSON_CreateNumber(in.a));
	return out;
}

static cJSON *serialize_color_node(const struct cr_color_node *in);
static cJSON *serialize_vector_node(const struct cr_vector_node *in);

static cJSON *serialize_value_node(const struct cr_value_node *in) {
	if (!in) return NULL;
	cJSON *out = cJSON_CreateObject();
	switch (in->type) {
		case cr_vn_unknown:
			cJSON_AddStringToObject(out, "type", "unknown");
			break;
		case cr_vn_constant:
			cJSON_AddStringToObject(out, "type", "constant");
			cJSON_AddItemToObject(out, "value", cJSON_CreateNumber(in->arg.constant));
			break;
		case cr_vn_fresnel:
			cJSON_AddStringToObject(out, "type", "fresnel");
			cJSON_AddItemToObject(out, "IOR", serialize_value_node(in->arg.fresnel.IOR));
			cJSON_AddItemToObject(out, "normal", serialize_vector_node(in->arg.fresnel.normal));
			break;
		case cr_vn_map_range:
			cJSON_AddStringToObject(out, "type", "map_range");
			cJSON_AddItemToObject(out, "input", serialize_value_node(in->arg.map_range.input_value));
			cJSON_AddItemToObject(out, "from_min", serialize_value_node(in->arg.map_range.from_min));
			cJSON_AddItemToObject(out, "from_max", serialize_value_node(in->arg.map_range.from_max));
			cJSON_AddItemToObject(out, "to_min", serialize_value_node(in->arg.map_range.to_min));
			cJSON_AddItemToObject(out, "to_max", serialize_value_node(in->arg.map_range.to_max));
			break;
		case cr_vn_raylength:
			cJSON_AddStringToObject(out, "type", "raylength");
			break;
		case cr_vn_alpha:
			cJSON_AddStringToObject(out, "type", "alpha");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.alpha.color));
			break;
		case cr_vn_vec_to_value:
			cJSON_AddStringToObject(out, "type", "vec_to_value");
			cJSON_AddItemToObject(out, "vector", serialize_vector_node(in->arg.vec_to_value.vec));
			cJSON_AddNumberToObject(out, "component", in->arg.vec_to_value.comp);
			break;
		case cr_vn_math:
			cJSON_AddStringToObject(out, "type", "math");
			cJSON_AddItemToObject(out, "a", serialize_value_node(in->arg.math.A));
			cJSON_AddItemToObject(out, "b", serialize_value_node(in->arg.math.B));
			cJSON_AddNumberToObject(out, "op", in->arg.math.op);
			break;
		case cr_vn_grayscale:
		default:
			cJSON_AddStringToObject(out, "type", "grayscale");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.grayscale.color));
			break;
	}
	return out;
}

static cJSON *serialize_vector_node(const struct cr_vector_node *in) {
	if (!in) return NULL;
	cJSON *out = cJSON_CreateObject();
	switch (in->type) {
		case cr_vec_unknown:
			cJSON_AddStringToObject(out, "type", "unknown");
			break;
		case cr_vec_constant:
			cJSON_AddStringToObject(out, "type", "constant");
			struct cr_vector c = in->arg.constant;
			cJSON_AddItemToObject(out, "vec", serialize_vector((struct vector){ c.x, c.y, c.z }));
			break;
		case cr_vec_normal:
			cJSON_AddStringToObject(out, "type", "normal");
			break;
		case cr_vec_uv:
			cJSON_AddStringToObject(out, "type", "uv");
			break;
		case cr_vec_vecmath:
			cJSON_AddStringToObject(out, "type", "vecmath");
			cJSON_AddItemToObject(out, "a", serialize_vector_node(in->arg.vecmath.A));
			cJSON_AddItemToObject(out, "b", serialize_vector_node(in->arg.vecmath.B));
			cJSON_AddItemToObject(out, "c", serialize_vector_node(in->arg.vecmath.C));
			cJSON_AddItemToObject(out, "f", serialize_value_node(in->arg.vecmath.f));
			cJSON_AddNumberToObject(out, "op", in->arg.vecmath.op);
			break;
		case cr_vec_mix:
			cJSON_AddStringToObject(out, "type", "mix");
			cJSON_AddItemToObject(out, "a", serialize_vector_node(in->arg.vec_mix.A));
			cJSON_AddItemToObject(out, "b", serialize_vector_node(in->arg.vec_mix.B));
			cJSON_AddItemToObject(out, "f", serialize_value_node(in->arg.vec_mix.factor));
			break;
	}
	return out;
}

static cJSON *serialize_color_node(const struct cr_color_node *in) {
	if (!in) return NULL;
	cJSON *out = cJSON_CreateObject();
	switch (in->type) {
		case cr_cn_unknown:
			cJSON_AddStringToObject(out, "type", "unknown");
			break;
		case cr_cn_constant:
			cJSON_AddStringToObject(out, "type", "constant");
			cJSON_AddItemToObject(out, "color", serialize_color(in->arg.constant));
			break;
		case cr_cn_image:
			cJSON_AddStringToObject(out, "type", "image");
			cJSON_AddStringToObject(out, "path", in->arg.image.full_path);
			cJSON_AddNumberToObject(out, "options", in->arg.image.options);
			break;
		case cr_cn_checkerboard:
			cJSON_AddStringToObject(out, "type", "checkerboard");
			cJSON_AddItemToObject(out, "color1", serialize_color_node(in->arg.checkerboard.a));
			cJSON_AddItemToObject(out, "color2", serialize_color_node(in->arg.checkerboard.b));
			cJSON_AddItemToObject(out, "scale", serialize_value_node(in->arg.checkerboard.scale));
			break;
		case cr_cn_blackbody:
			cJSON_AddStringToObject(out, "type", "blackbody");
			cJSON_AddItemToObject(out, "degrees", serialize_value_node(in->arg.blackbody.degrees));
			break;
		case cr_cn_split:
			cJSON_AddStringToObject(out, "type", "split");
			cJSON_AddItemToObject(out, "constant", serialize_value_node(in->arg.split.node));
			break;
		case cr_cn_rgb:
			cJSON_AddStringToObject(out, "type", "rgb");
			cJSON_AddItemToObject(out, "r", serialize_value_node(in->arg.rgb.red));
			cJSON_AddItemToObject(out, "g", serialize_value_node(in->arg.rgb.green));
			cJSON_AddItemToObject(out, "b", serialize_value_node(in->arg.rgb.blue));
			break;
		case cr_cn_hsl:
			cJSON_AddStringToObject(out, "type", "hsl");
			cJSON_AddItemToObject(out, "h", serialize_value_node(in->arg.hsl.H));
			cJSON_AddItemToObject(out, "s", serialize_value_node(in->arg.hsl.S));
			cJSON_AddItemToObject(out, "l", serialize_value_node(in->arg.hsl.L));
			break;
		case cr_cn_hsv:
			cJSON_AddStringToObject(out, "type", "hsv");
			cJSON_AddItemToObject(out, "h", serialize_value_node(in->arg.hsv.H));
			cJSON_AddItemToObject(out, "s", serialize_value_node(in->arg.hsv.S));
			cJSON_AddItemToObject(out, "v", serialize_value_node(in->arg.hsv.V));
			break;
		case cr_cn_hsv_tform:
			cJSON_AddStringToObject(out, "type", "hsv_tform");
			cJSON_AddItemToObject(out, "tex", serialize_color_node(in->arg.hsv_tform.tex));
			cJSON_AddItemToObject(out, "h", serialize_value_node(in->arg.hsv_tform.H));
			cJSON_AddItemToObject(out, "s", serialize_value_node(in->arg.hsv_tform.S));
			cJSON_AddItemToObject(out, "v", serialize_value_node(in->arg.hsv_tform.V));
			cJSON_AddItemToObject(out, "f", serialize_value_node(in->arg.hsv_tform.f));
			break;
		case cr_cn_vec_to_color:
			cJSON_AddStringToObject(out, "type", "to_color");
			cJSON_AddItemToObject(out, "vector", serialize_vector_node(in->arg.vec_to_color.vec));
			break;
		case cr_cn_gradient:
			cJSON_AddStringToObject(out, "type", "gradient");
			cJSON_AddItemToObject(out, "down", serialize_color_node(in->arg.gradient.a));
			cJSON_AddItemToObject(out, "up", serialize_color_node(in->arg.gradient.b));
			break;
		case cr_cn_color_mix:
			cJSON_AddStringToObject(out, "type", "color_mix");
			cJSON_AddItemToObject(out, "a", serialize_color_node(in->arg.color_mix.a));
			cJSON_AddItemToObject(out, "b", serialize_color_node(in->arg.color_mix.b));
			cJSON_AddItemToObject(out, "factor", serialize_value_node(in->arg.color_mix.factor));
			break;
	}
	return out;
}

cJSON *serialize_shader_node(const struct cr_shader_node *in) {
	if (!in) return NULL;
	cJSON *out = cJSON_CreateObject();
	switch (in->type) {
		case cr_bsdf_unknown:
			cJSON_AddStringToObject(out, "type", "unknown");
			break;
		case cr_bsdf_diffuse:
			cJSON_AddStringToObject(out, "type", "diffuse");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.diffuse.color));
			break;
		case cr_bsdf_metal:
			cJSON_AddStringToObject(out, "type", "metal");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.metal.color));
			cJSON_AddItemToObject(out, "roughness", serialize_value_node(in->arg.metal.roughness));
			break;
		case cr_bsdf_glass:
			cJSON_AddStringToObject(out, "type", "glass");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.glass.color));
			cJSON_AddItemToObject(out, "roughness", serialize_value_node(in->arg.glass.roughness));
			cJSON_AddItemToObject(out, "IOR", serialize_value_node(in->arg.glass.IOR));
			break;
		case cr_bsdf_plastic:
			cJSON_AddStringToObject(out, "type", "plastic");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.plastic.color));
			cJSON_AddItemToObject(out, "roughness", serialize_value_node(in->arg.plastic.roughness));
			cJSON_AddItemToObject(out, "IOR", serialize_value_node(in->arg.plastic.IOR));
			break;
		case cr_bsdf_mix:
			cJSON_AddStringToObject(out, "type", "mix");
			cJSON_AddItemToObject(out, "A", serialize_shader_node(in->arg.mix.A));
			cJSON_AddItemToObject(out, "B", serialize_shader_node(in->arg.mix.B));
			cJSON_AddItemToObject(out, "factor", serialize_value_node(in->arg.mix.factor));
			break;
		case cr_bsdf_add:
			cJSON_AddStringToObject(out, "type", "add");
			cJSON_AddItemToObject(out, "A", serialize_shader_node(in->arg.add.A));
			cJSON_AddItemToObject(out, "B", serialize_shader_node(in->arg.add.B));
			break;
		case cr_bsdf_transparent:
			cJSON_AddStringToObject(out, "type", "transparent");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.transparent.color));
			break;
		case cr_bsdf_emissive:
			cJSON_AddStringToObject(out, "type", "emissive");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.emissive.color));
			cJSON_AddItemToObject(out, "strength", serialize_value_node(in->arg.emissive.strength));
			break;
		case cr_bsdf_translucent:
			cJSON_AddStringToObject(out, "type", "translucent");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.translucent.color));
			break;
		case cr_bsdf_background:
			cJSON_AddStringToObject(out, "type", "background");
			cJSON_AddItemToObject(out, "color", serialize_color_node(in->arg.background.color));
			cJSON_AddItemToObject(out, "offset", serialize_vector_node(in->arg.background.pose));
			cJSON_AddItemToObject(out, "strength", serialize_value_node(in->arg.background.strength));
			break;
	}
	return out;
}

struct cr_shader_node *deserialize_shader_node(const cJSON *in) {
	return cr_shader_node_build(in);
}

static cJSON *serialize_scene(const struct world *in) {
	cJSON *out = cJSON_CreateObject();

	cJSON_AddStringToObject(out, "asset_path", in->asset_path);

	cJSON_AddItemToObject(out, "background", serialize_shader_node(in->bg_desc));

	cJSON *textures = cJSON_CreateArray();
	for (size_t i = 0; i < in->textures.count; ++i) {
		cJSON *asset = cJSON_CreateObject();
		cJSON_AddItemToObject(asset, "p", cJSON_CreateString(in->textures.items[i].path));
		cJSON_AddItemToObject(asset, "t", serialize_texture(in->textures.items[i].t));
		cJSON_AddItemToArray(textures, asset);
	}
	cJSON_AddItemToObject(out, "textures", textures);

	cJSON *v_buffers = cJSON_CreateArray();
	for (size_t i = 0; i < in->v_buffers.count; ++i) {
		cJSON_AddItemToArray(v_buffers, serialize_vertex_buffer(in->v_buffers.items[i]));
	}
	cJSON_AddItemToObject(out, "v_buffers", v_buffers);

	// Note: We only really need the descriptions, since we can't serialize the actual shaders anyway
	cJSON *shader_buffers = cJSON_CreateArray();
	for (size_t i = 0; i < in->shader_buffers.count; ++i) {
		cJSON *descriptions = cJSON_CreateArray();
		for (size_t j = 0; j < in->shader_buffers.items[i].descriptions.count; ++j) {
			cJSON_AddItemToArray(descriptions, serialize_shader_node(in->shader_buffers.items[i].descriptions.items[j]));
		}
		cJSON_AddItemToArray(shader_buffers, descriptions);
	}
	cJSON_AddItemToObject(out, "shader_buffers", shader_buffers);

	cJSON *meshes = cJSON_CreateArray();
	for (size_t i = 0; i < in->meshes.count; ++i) {
		cJSON_AddItemToArray(meshes, serialize_mesh(in->meshes.items[i]));
	}
	cJSON_AddItemToObject(out, "meshes", meshes);

	cJSON *spheres = cJSON_CreateArray();
	for (size_t i = 0; i < in->spheres.count; ++i) {
		cJSON_AddItemToArray(spheres, serialize_sphere(in->spheres.items[i]));
	}
	cJSON_AddItemToObject(out, "spheres", spheres);

	cJSON *instances = cJSON_CreateArray();
	for (size_t i = 0; i < in->instances.count; ++i) {
		cJSON_AddItemToArray(instances, serialize_instance(in->instances.items[i]));
	}
	cJSON_AddItemToObject(out, "instances", instances);

	cJSON *cameras = cJSON_CreateArray();
	for (size_t i = 0; i < in->cameras.count; ++i) {
		cJSON_AddItemToArray(cameras, serialize_camera(in->cameras.items[i]));
	}
	cJSON_AddItemToObject(out, "cameras", cameras);

	return out;
}

struct world *deserialize_scene(const cJSON *in) {
	if (!in) return NULL;
	struct world *out = calloc(1, sizeof(*out));

	out->asset_path = stringCopy("./");
	out->storage.node_pool = newBlock(NULL, 1024);
	out->storage.node_table = newHashtable(compareNodes, &out->storage.node_pool);

	cJSON *asset_path = cJSON_GetObjectItem(in, "asset_path");
	if (cJSON_IsString(asset_path)) {
		if (out->asset_path) free(out->asset_path);
		out->asset_path = stringCopy(asset_path->valuestring);
	}

	const cJSON *background = cJSON_GetObjectItem(in, "background");
	if (cJSON_IsObject(background)) {
		out->bg_desc = deserialize_shader_node(background);
		out->background = build_bsdf_node((struct cr_scene *)out, out->bg_desc);
	}
	const cJSON *textures = cJSON_GetObjectItem(in, "textures");
	if (cJSON_IsArray(textures)) {
		cJSON *texture = NULL;
		cJSON_ArrayForEach(texture, textures) {
			texture_asset_arr_add(&out->textures, (struct texture_asset){
				.path = stringCopy(cJSON_GetStringValue(cJSON_GetObjectItem(texture, "p"))),
				.t = deserialize_texture(cJSON_GetObjectItem(texture, "t"))
			});
		}
	}
	const cJSON *v_buffers = cJSON_GetObjectItem(in, "v_buffers");
	if (cJSON_IsArray(v_buffers)) {
		cJSON *v_buffer = NULL;
		cJSON_ArrayForEach(v_buffer, v_buffers) {
			vertex_buffer_arr_add(&out->v_buffers, deserialize_vertex_buffer(v_buffer));
		}
	}
	const cJSON *shader_buffers = cJSON_GetObjectItem(in, "shader_buffers");
	if (cJSON_IsArray(shader_buffers)) {
		cJSON *s_buffer = NULL;
		cJSON_ArrayForEach(s_buffer, shader_buffers) {
			size_t idx = bsdf_buffer_arr_add(&out->shader_buffers, (struct bsdf_buffer){ 0 });
			struct bsdf_buffer *buf = &out->shader_buffers.items[idx];
			if (cJSON_IsArray(s_buffer)) {
				cJSON *description = NULL;
				cJSON_ArrayForEach(description, s_buffer) {
					struct cr_shader_node *desc = deserialize_shader_node(description);
					cr_shader_node_ptr_arr_add(&buf->descriptions, desc);
					bsdf_node_ptr_arr_add(&buf->bsdfs, build_bsdf_node((struct cr_scene *)out, desc));
				}
			}
		}
	}

	cJSON *meshes = cJSON_GetObjectItem(in, "meshes");
	if (cJSON_IsArray(meshes)) {
		cJSON *mesh = NULL;
		cJSON_ArrayForEach(mesh, meshes) {
			mesh_arr_add(&out->meshes, deserialize_mesh(mesh));
		}
	}

	// Hook up vertex buffers to meshes
	for (size_t i = 0; i < out->meshes.count; ++i) {
		struct mesh *m = &out->meshes.items[i];
		m->vbuf = &out->v_buffers.items[m->vbuf_idx];
	}

	cJSON *spheres = cJSON_GetObjectItem(in, "spheres");
	if (cJSON_IsArray(spheres)) {
		cJSON *sphere = NULL;
		cJSON_ArrayForEach(sphere, spheres) {
			sphere_arr_add(&out->spheres, deserialize_sphere(sphere));
		}
	}
	cJSON *instances = cJSON_GetObjectItem(in, "instances");
	if (cJSON_IsArray(instances)) {
		cJSON *instance = NULL;
		cJSON_ArrayForEach(instance, instances) {
			instance_arr_add(&out->instances, deserialize_instance(instance));
		}
	}

	// Hook up shader buffers and object arrays to instances
	for (size_t i = 0; i < out->instances.count; ++i) {
		struct instance *inst = &out->instances.items[i];
		inst->bbuf = &out->shader_buffers.items[inst->bbuf_idx];
		if (isMesh(inst)) {
			inst->object_arr = &out->meshes;
		} else {
			inst->object_arr = &out->spheres;
		}
	}

	cJSON *cameras = cJSON_GetObjectItem(in, "cameras");
	if (cJSON_IsArray(cameras)) {
		cJSON *camera = NULL;
		cJSON_ArrayForEach(camera, cameras) {
			camera_arr_add(&out->cameras, deserialize_camera(camera));
		}
	}

	return out;
}

static cJSON *serialize_prefs(const struct prefs in) {
	cJSON *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "samples", cJSON_CreateNumber(in.sampleCount));
	cJSON_AddItemToObject(out, "bounces", cJSON_CreateNumber(in.bounces));
	cJSON_AddItemToObject(out, "tileWidth", cJSON_CreateNumber(in.tileWidth));
	cJSON_AddItemToObject(out, "tileHeight", cJSON_CreateNumber(in.tileHeight));
	cJSON_AddItemToObject(out, "tileOrder", cJSON_CreateNumber(in.tileOrder));
	cJSON_AddItemToObject(out, "outputFilePath", cJSON_CreateString(in.imgFilePath));
	cJSON_AddItemToObject(out, "outputFileName", cJSON_CreateString(in.imgFileName));
	cJSON_AddItemToObject(out, "count", cJSON_CreateNumber(in.imgCount));
	cJSON_AddItemToObject(out, "width", cJSON_CreateNumber(in.override_width));
	cJSON_AddItemToObject(out, "height", cJSON_CreateNumber(in.override_height));
	cJSON_AddItemToObject(out, "selected_camera", cJSON_CreateNumber(in.selected_camera));
	return out;
}

struct prefs deserialize_prefs(const cJSON *in) {
	struct prefs p = default_prefs();
	if (!in) return p;
	p.sampleCount = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "samples"));
	p.bounces = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "bounces"));
	p.tileWidth = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "tileWidth"));
	p.tileHeight = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "tileHeight"));
	p.tileOrder = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "tileOrder"));
	free(p.imgFilePath);
	free(p.imgFileName);
	p.imgFilePath = stringCopy(cJSON_GetStringValue(cJSON_GetObjectItem(in, "outputFilePath")));
	p.imgFileName = stringCopy(cJSON_GetStringValue(cJSON_GetObjectItem(in, "outputFileName")));
	p.imgCount = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "count"));
	p.override_width = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "width"));
	p.override_height = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "height"));
	p.selected_camera = cJSON_GetNumberValue(cJSON_GetObjectItem(in, "selected_camera"));
	return p;
}

static cJSON *serialize_json(const struct renderer *r) {
	if (!r) return NULL;
	cJSON *out = cJSON_CreateObject();
	cJSON_AddItemToObject(out, "scene", serialize_scene(r->scene));
	cJSON_AddItemToObject(out, "prefs", serialize_prefs(r->prefs));
	return out;
}

char *serialize_renderer(const struct renderer *r) {
	if (!r) return NULL;
	cJSON *out = serialize_json(r);
	char *data = cJSON_PrintUnformatted(out);
	cJSON_Delete(out);
	return data;
}

void dump_renderer_state(const struct renderer *r) {
	if (!r) return;
	cJSON *out = serialize_json(r);
	printf("%s\n", cJSON_Print(out));
	cJSON_Delete(out);
}

struct renderer *deserialize_renderer(const char *data) {
	cJSON *renderer = cJSON_Parse(data);
	if (!renderer) return NULL;
	struct renderer *r = calloc(1, sizeof(*r));
	r->state.finishedPasses = 1;
	r->scene = deserialize_scene(cJSON_GetObjectItem(renderer, "scene"));
	r->prefs = deserialize_prefs(cJSON_GetObjectItem(renderer, "prefs"));
	cJSON_Delete(renderer);
	return r;
}

#else
// Empty stub for Windows
void dump_renderer_state(const struct renderer *r) {
	(void)r;
}
#endif
