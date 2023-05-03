//
//  valuenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct constantValue *self = (struct constantValue *)node;
	snprintf(dumpbuf, bufsize, "constantValue { value: %.2f }", (double)self->value);
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)sampler;
	struct constantValue *this = (struct constantValue *)node;
	return this->value;
}

const struct valueNode *newConstantValue(const struct node_storage *s, float value) {
	HASH_CONS(s->node_table, hash, struct constantValue, {
		.value = value,
		.node = {
			.eval = eval,
			.constant = true,
			.base = { .compare = compare, .dump = dump }
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

static enum mathOp parseMathOp(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No math op given, defaulting to add.\n");
		return Add;
	}
	if (stringEquals(data->valuestring, "add")) return Add;
	if (stringEquals(data->valuestring, "subtract")) return Subtract;
	if (stringEquals(data->valuestring, "multiply")) return Multiply;
	if (stringEquals(data->valuestring, "divide")) return Divide;
	if (stringEquals(data->valuestring, "power")) return Power;
	if (stringEquals(data->valuestring, "log")) return Log;
	if (stringEquals(data->valuestring, "sqrt")) return SquareRoot;
	if (stringEquals(data->valuestring, "invsqrt")) return InvSquareRoot;
	if (stringEquals(data->valuestring, "abs")) return Absolute;
	if (stringEquals(data->valuestring, "min")) return Min;
	if (stringEquals(data->valuestring, "max")) return Max;
	if (stringEquals(data->valuestring, "lt")) return LessThan;
	if (stringEquals(data->valuestring, "gt")) return GreaterThan;
	if (stringEquals(data->valuestring, "sign")) return Sign;
	if (stringEquals(data->valuestring, "compare")) return Compare;
	if (stringEquals(data->valuestring, "round")) return Round;
	if (stringEquals(data->valuestring, "floor")) return Floor;
	if (stringEquals(data->valuestring, "ceil")) return Ceil;
	if (stringEquals(data->valuestring, "truncate")) return Truncate;
	if (stringEquals(data->valuestring, "fraction")) return Fraction;
	if (stringEquals(data->valuestring, "mod")) return Modulo;
	if (stringEquals(data->valuestring, "sin")) return Sine;
	if (stringEquals(data->valuestring, "cos")) return Cosine;
	if (stringEquals(data->valuestring, "tan")) return Tangent;
	if (stringEquals(data->valuestring, "toradians")) return ToRadians;
	if (stringEquals(data->valuestring, "todegrees")) return ToDegrees;
	logr(warning, "Unknown math op %s given, defaulting to add\n", data->valuestring);
	return Add;
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
			if (!asset_path || !cache) {
				ASSERT_NOT_REACHED(); //FIXME
			}
			const struct colorNode *color = parseTextureNode(asset_path, cache, s, cJSON_GetObjectItem(node, "color"));
			return newAlpha(s, color);
		}
		if (stringEquals(type->valuestring, "vec_to_value")) {
			const struct vectorNode *vec = parseVectorNode(s, cJSON_GetObjectItem(node, "vector"));
			enum component comp = parseComponent(cJSON_GetObjectItem(node, "component"));
			return newVecToValue(s, vec, comp);
		}
		if (stringEquals(type->valuestring, "math")) {
			enum mathOp op = parseMathOp(cJSON_GetObjectItem(node, "op"));
			const struct valueNode *A = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "a"));
			const struct valueNode *B = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "b"));
			return newMath(s, A, B, op);
		}
	}

	return newGrayscaleConverter(s, parseTextureNode(asset_path, cache, s, node));
}