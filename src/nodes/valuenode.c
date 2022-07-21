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

const struct valueNode *newConstantValue(const struct world *world, float value) {
	HASH_CONS(world->nodeTable, hash, struct constantValue, {
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

const struct valueNode *parseValueNode(struct renderer *r, const cJSON *node) {
	if (!node) return NULL;
	struct world *w = r->scene;
	if (cJSON_IsNumber(node)) {
		return newConstantValue(w, node->valuedouble);
	}

	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (cJSON_IsString(type)) {
		const struct valueNode *IOR = parseValueNode(r, cJSON_GetObjectItem(node, "IOR"));
		const struct vectorNode *normal = parseVectorNode(w, cJSON_GetObjectItem(node, "normal"));
		const struct colorNode *color = parseTextureNode(r, cJSON_GetObjectItem(node, "color"));

		if (stringEquals(type->valuestring, "fresnel")) {
			return newFresnel(w, IOR, normal);
		}
		if (stringEquals(type->valuestring, "map_range")) {
			const struct valueNode *input_value = parseValueNode(r, cJSON_GetObjectItem(node, "input"));
			const struct valueNode *from_min = parseValueNode(r, cJSON_GetObjectItem(node, "from_min"));
			const struct valueNode *from_max = parseValueNode(r, cJSON_GetObjectItem(node, "from_max"));
			const struct valueNode *to_min = parseValueNode(r, cJSON_GetObjectItem(node, "to_min"));
			const struct valueNode *to_max = parseValueNode(r, cJSON_GetObjectItem(node, "to_max"));
			return newMapRange(w, input_value, from_min, from_max, to_min, to_max);
		}
		if (stringEquals(type->valuestring, "raylength")) {
			return newRayLength(w);
		}
		if (stringEquals(type->valuestring, "alpha")) {
			return newAlpha(w, color);
		}
		if (stringEquals(type->valuestring, "vec_to_value")) {
			const struct vectorNode *vec = parseVectorNode(r->scene, cJSON_GetObjectItem(node, "vector"));
			enum component comp = parseComponent(cJSON_GetObjectItem(node, "component"));
			return newVecToValue(w, vec, comp);
		}
	}

	return newGrayscaleConverter(w, parseTextureNode(r, node));
}