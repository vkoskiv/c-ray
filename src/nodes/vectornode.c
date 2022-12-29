//
//  vectornode.c
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
#include "bsdfnode.h"

#include "nodes/valuenode.h"
#include "vectornode.h"

struct constantVector {
	struct vectorNode node;
	struct vector vector;
};

static bool compare(const void *A, const void *B) {
	const struct constantVector *this = A;
	const struct constantVector *other = B;
	return vecEquals(this->vector, other->vector);
}

static uint32_t hash(const void *p) {
	const struct constantVector *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->vector, sizeof(this->vector));
	return h;
}

static struct vectorValue eval(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)sampler;
	struct constantVector *this = (struct constantVector *)node;
	return (struct vectorValue){ .v = this->vector };
}

const struct vectorNode *newConstantVector(const struct node_storage *s, const struct vector vector) {
	HASH_CONS(s->node_table, hash, struct constantVector, {
		.vector = vector,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}

struct constantUV {
	struct vectorNode node;
	struct coord uv;
};

static bool compare_uv(const void *A, const void *B) {
	const struct constantUV *this = A;
	const struct constantUV *other = B;
	return this->uv.x == other->uv.x && this->uv.y == other->uv.y;
}

static uint32_t hash_uv(const void *p) {
	const struct constantUV *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->uv, sizeof(this->uv));
	return h;
}

static struct vectorValue eval_uv(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)record;
	(void)sampler;
	struct constantUV *this = (struct constantUV *)node;
	return (struct vectorValue){ .c = this->uv };
}

const struct vectorNode *newConstantUV(const struct node_storage *s, const struct coord c) {
	HASH_CONS(s->node_table, hash_uv, struct constantUV, {
		.uv = c,
		.node = {
			.eval = eval_uv,
			.base = { .compare = compare_uv }
		}
	});
}

static enum vecOp parseVectorOp(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No vector op given, defaulting to add.\n");
		return VecAdd;
	}
	if (stringEquals(data->valuestring, "add")) return VecAdd;
	if (stringEquals(data->valuestring, "subtract")) return VecSubtract;
	if (stringEquals(data->valuestring, "multiply")) return VecMultiply;
	if (stringEquals(data->valuestring, "divide")) return VecDivide;
	if (stringEquals(data->valuestring, "cross")) return VecCross;
	if (stringEquals(data->valuestring, "reflect")) return VecReflect;
	if (stringEquals(data->valuestring, "refract")) return VecRefract;
	if (stringEquals(data->valuestring, "dot")) return VecDot;
	if (stringEquals(data->valuestring, "distance")) return VecDistance;
	if (stringEquals(data->valuestring, "length")) return VecLength;
	if (stringEquals(data->valuestring, "scale")) return VecScale;
	if (stringEquals(data->valuestring, "normalize")) return VecNormalize;
	if (stringEquals(data->valuestring, "wrap")) return VecWrap;
	if (stringEquals(data->valuestring, "floor")) return VecFloor;
	if (stringEquals(data->valuestring, "ceil")) return VecCeil;
	if (stringEquals(data->valuestring, "mod")) return VecModulo;
	if (stringEquals(data->valuestring, "abs")) return VecAbs;
	if (stringEquals(data->valuestring, "min")) return VecMin;
	if (stringEquals(data->valuestring, "max")) return VecMax;
	if (stringEquals(data->valuestring, "sin")) return VecSin;
	if (stringEquals(data->valuestring, "cos")) return VecCos;
	if (stringEquals(data->valuestring, "tan")) return VecTan;
	logr(warning, "Unknown vector op %s given, defaulting to add\n", data->valuestring);
	return VecAdd;
}

struct vector parseVector(const struct cJSON *data) {
	const float x = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
	const float y = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
	const float z = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
	return (struct vector){ x, y, z };
}
const struct vectorNode *parseVectorNode(struct node_storage *s, const struct cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsArray(node)) {
		return newConstantVector(s, parseVector(node));
	}
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for vectorNode: %s\n", cJSON_PrintUnformatted(node));
		return newConstantVector(s, vecZero());
	}

	if (stringEquals(type->valuestring, "normal")) return newNormal(s);
	if (stringEquals(type->valuestring, "uv")) return newUV(s);

	const struct vectorNode *a = parseVectorNode(s, cJSON_GetObjectItem(node, "a"));
	const struct vectorNode *b = parseVectorNode(s, cJSON_GetObjectItem(node, "b"));
	const struct vectorNode *c = parseVectorNode(s, cJSON_GetObjectItem(node, "c"));
	//FIXME: alpha won't work here, for now.
	const struct valueNode  *f = parseValueNode(NULL, NULL, s, cJSON_GetObjectItem(node, "f"));
	if (stringEquals(type->valuestring, "uv_to_vec")) return new_uv_to_vec(s, a);
	if (stringEquals(type->valuestring, "vecmath")) {
		const enum vecOp op = parseVectorOp(cJSON_GetObjectItem(node, "op"));
		return newVecMath(s, a, b, c, f, op);
	}
	if (stringEquals(type->valuestring, "mix")) return new_vec_mix(s, a, b, f);
	return NULL;
}