//
//  vectornode.c
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
#include "bsdfnode.h"

#include "valuenode.h"
#include "vectornode.h"

struct constantVector {
	struct vectorNode node;
	struct vector vector;
};

static bool compare(const void *A, const void *B) {
	const struct constantVector *this = A;
	const struct constantVector *other = B;
	return vec_equals(this->vector, other->vector);
}

static uint32_t hash(const void *p) {
	const struct constantVector *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->vector, sizeof(this->vector));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct constantVector *self = (struct constantVector *)node;
	snprintf(dumpbuf, bufsize, "constantVector { %.2f, %.2f, %.2f }", (double)self->vector.x, (double)self->vector.y, (double)self->vector.z);
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
			.base = { .compare = compare, .dump = dump }
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

static void dump_uv(const void *node, char *dumpbuf, int bufsize) {
	struct constantUV *self = (struct constantUV *)node;
	snprintf(dumpbuf, bufsize, "constantUV { %.2f, %.2f }", (double)self->uv.x, (double)self->uv.y);
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
			.base = { .compare = compare_uv, .dump = dump_uv }
		}
	});
}

const struct vectorNode *build_vector_node(struct cr_scene *s_ext, const struct cr_vector_node *desc) {
	if (!s_ext || !desc) return NULL;
	struct world *scene = (struct world *)s_ext;
	struct node_storage s = scene->storage;

	switch (desc->type) {
		case cr_vec_constant:
			return newConstantVector(&s, (struct vector){ desc->arg.constant.x, desc->arg.constant.y, desc->arg.constant.z });
		case cr_vec_normal:
			return newNormal(&s);
		case cr_vec_uv:
			return newUV(&s);
		case cr_vec_vecmath:
			return newVecMath(&s,
				build_vector_node(s_ext, desc->arg.vecmath.A),
				build_vector_node(s_ext, desc->arg.vecmath.B),
				build_vector_node(s_ext, desc->arg.vecmath.C),
				build_value_node(s_ext, desc->arg.vecmath.f),
				desc->arg.vecmath.op);
		case cr_vec_mix:
			return new_vec_mix(&s,
				build_vector_node(s_ext, desc->arg.vec_mix.A),
				build_vector_node(s_ext, desc->arg.vec_mix.B),
				build_value_node(s_ext, desc->arg.vec_mix.factor));
		default:
			return NULL;
	};
}

