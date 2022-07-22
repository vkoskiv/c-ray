//
//  map_range.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 11/08/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../nodebase.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
#include "../valuenode.h"

#include "map_range.h"

struct mapRangeNode {
	struct valueNode node;
	const struct valueNode *input_value;
	const struct valueNode *from_min;
	const struct valueNode *from_max;
	const struct valueNode *to_min;
	const struct valueNode *to_max;
};

static bool compare(const void *A, const void *B) {
	const struct mapRangeNode *this = A;
	const struct mapRangeNode *other = B;
	return this->input_value == other->input_value &&
	this->from_min == other->from_min &&
	this->from_max == other->from_max &&
	this->to_min == other->to_min &&
	this->to_max == other->to_max;
}

static uint32_t hash(const void *p) {
	const struct mapRangeNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->input_value, sizeof(this->input_value));
	h = hashBytes(h, &this->from_min, sizeof(this->from_min));
	h = hashBytes(h, &this->from_max, sizeof(this->from_max));
	h = hashBytes(h, &this->to_min, sizeof(this->to_min));
	h = hashBytes(h, &this->to_max, sizeof(this->to_max));
	return h;
}

static inline float lerp(float min, float max, float t) {
	return ((1.0f - t) * min) + (t * max);
}

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	const struct mapRangeNode *this = (const struct mapRangeNode *)node;
	const float input_value = this->input_value->eval(this->input_value, record);
	
	const float from_min = this->from_min->eval(this->from_min, record);
	const float from_max =  this->from_max->eval(this->from_max, record);
	
	const float delta = from_max - from_min;
	const float t = clamp(input_value / delta, 0.0f, 1.0f);
	
	const float to_min = this->to_min->eval(this->to_min, record);
	const float to_max = this->to_max->eval(this->to_max, record);
	
	return lerp(to_min, to_max, t);
}

const struct valueNode *newMapRange(const struct node_storage *s,
									const struct valueNode *input_value,
									const struct valueNode *from_min,
									const struct valueNode *from_max,
									const struct valueNode *to_min,
									const struct valueNode *to_max) {
	HASH_CONS(s->node_table, hash, struct mapRangeNode, {
		.input_value = input_value ? input_value : newConstantValue(s, 1.0f),
		.from_min = from_min ? from_min : newConstantValue(s, 0.0f),
		.from_max = from_max ? from_max : newConstantValue(s, 1.0f),
		.to_min = to_min ? to_min : newConstantValue(s, 0.0f),
		.to_max = to_max ? to_max : newConstantValue(s, 1.0f),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
