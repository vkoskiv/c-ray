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

const struct valueNode *build_value_node(struct cr_renderer *r_ext, const struct cr_value_node *desc) {
	if (!r_ext || !desc) return NULL;
	struct renderer *r = (struct renderer *)r_ext;
	struct node_storage s = r->scene->storage;

	switch (desc->type) {
		case cr_vn_constant:
			return newConstantValue(&s, desc->arg.constant);
		case cr_vn_fresnel:
			return newFresnel(&s, build_value_node(r_ext, desc->arg.fresnel.IOR), build_vector_node(r_ext, desc->arg.fresnel.normal));
		case cr_vn_map_range:
			return newMapRange(&s,
				build_value_node(r_ext, desc->arg.map_range.input_value),
				build_value_node(r_ext, desc->arg.map_range.from_min),
				build_value_node(r_ext, desc->arg.map_range.from_max),
				build_value_node(r_ext, desc->arg.map_range.to_min),
				build_value_node(r_ext, desc->arg.map_range.to_max));
		case cr_vn_raylength:
			return newRayLength(&s);
		case cr_vn_alpha:
			return newAlpha(&s, build_color_node(r_ext, desc->arg.alpha.color));
		case cr_vn_vec_to_value:
			return newVecToValue(&s, build_vector_node(r_ext, desc->arg.vec_to_value.vec), desc->arg.vec_to_value.comp);
		case cr_vn_math:
			return newMath(&s,
				build_value_node(r_ext, desc->arg.math.A),
				build_value_node(r_ext, desc->arg.math.B),
				desc->arg.math.op);
		case cr_vn_grayscale:
			return newGrayscaleConverter(&s, build_color_node(r_ext, desc->arg.grayscale.color));
		default:
			return NULL;
	}
}
