//
//  valuenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/color.h"
#include "../datatypes/vector.h"
#include "../datatypes/hitrecord.h"
#include "../datatypes/scene.h"
#include "../utils/hashtable.h"
#include "../utils/string.h"
#include "../renderer/renderer.h"

#include "vectornode.h"
#include "valuenode.h"

struct constantValue {
	struct valueNode node;
	float value;
};

static bool compare(const void *A, const void *B) {
	const struct constantValue *this = A;
	const struct constantValue *other = B;
	return this->value == other->value;
}

static uint32_t hash(const void *p) {
	const struct constantValue *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->value, sizeof(this->value));
	return h;
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	(void)record;
	struct constantValue *this = (struct constantValue *)node;
	return this->value;
}

const struct valueNode *newConstantValue(const struct node_storage *s, float value) {
	HASH_CONS(s->node_table, hash, struct constantValue, {
		.value = value,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}

static enum component parseComponent(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No component specified for vecToValue node, defaulting to scalar (F).\n");
		return F;
	}
	if (stringEquals(data->valuestring, "x")) return X;
	if (stringEquals(data->valuestring, "y")) return Y;
	if (stringEquals(data->valuestring, "z")) return Z;
	if (stringEquals(data->valuestring, "u")) return U;
	if (stringEquals(data->valuestring, "v")) return V;
	if (stringEquals(data->valuestring, "f")) return F;
	logr(warning, "Unknown component specified: %s\n", data->valuestring);
	return F;
}

const struct valueNode *parseValueNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsNumber(node)) {
		return newConstantValue(s, node->valuedouble);
	}

	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (cJSON_IsString(type)) {
		const struct valueNode *IOR = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "IOR"));
		const struct vectorNode *normal = parseVectorNode(s, cJSON_GetObjectItem(node, "normal"));
		const struct colorNode *color = parseTextureNode(asset_path, cache, s, cJSON_GetObjectItem(node, "color"));

		if (stringEquals(type->valuestring, "fresnel")) {
			return newFresnel(s, IOR, normal);
		}
		if (stringEquals(type->valuestring, "map_range")) {
			const struct valueNode *input_value = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "input"));
			const struct valueNode *from_min = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "from_min"));
			const struct valueNode *from_max = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "from_max"));
			const struct valueNode *to_min = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "to_min"));
			const struct valueNode *to_max = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "to_max"));
			return newMapRange(s, input_value, from_min, from_max, to_min, to_max);
		}
		if (stringEquals(type->valuestring, "raylength")) {
			return newRayLength(s);
		}
		if (stringEquals(type->valuestring, "alpha")) {
			return newAlpha(s, color);
		}
		if (stringEquals(type->valuestring, "vec_to_value")) {
			const struct vectorNode *vec = parseVectorNode(s, cJSON_GetObjectItem(node, "vector"));
			enum component comp = parseComponent(cJSON_GetObjectItem(node, "component"));
			return newVecToValue(s, vec, comp);
		}
	}

	return newGrayscaleConverter(s, parseTextureNode(asset_path, cache, s, node));
}