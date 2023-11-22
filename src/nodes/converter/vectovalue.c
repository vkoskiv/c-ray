//
//  vectovalue.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../nodebase.h"

#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/hitrecord.h"
#include "../vectornode.h"

#include "vectovalue.h"

struct vecToValueNode {
	struct valueNode node;
	const struct vectorNode *vec;
	enum cr_vec_to_value_component component_to_get;
};

static bool compare(const void *A, const void *B) {
	const struct vecToValueNode *this = A;
	const struct vecToValueNode *other = B;
	return this->vec == other->vec && this->component_to_get == other->component_to_get;
}

static uint32_t hash(const void *p) {
	const struct vecToValueNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->vec, sizeof(this->vec));
	h = hashBytes(h, &this->component_to_get, sizeof(this->component_to_get));
	return h;
}

char component_to_char(enum cr_vec_to_value_component c) {
	switch (c) {
		case X: return 'X';
		case Y: return 'Y';
		case Z: return 'Z';
		case U: return 'U';
		case V: return 'V';
		case F: return 'F';
	}
	return '?';
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct vecToValueNode *self = (struct vecToValueNode *)node;
	char vec[DUMPBUF_SIZE / 2] = "";
	if (self->vec->base.dump) self->vec->base.dump(self->vec, vec, sizeof(vec));
	snprintf(dumpbuf, bufsize, "vecToValueNode { vec: %s, component: %c }", vec, component_to_char(self->component_to_get));
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	struct vecToValueNode *this = (struct vecToValueNode *)node;
	const struct vectorValue val = this->vec->eval(this->vec, sampler, record);
	switch (this->component_to_get) {
		case X: return val.v.x;
		case Y: return val.v.y;
		case Z: return val.v.z;
		case U: return val.c.x;
		case V: return val.c.y;
		case F: return val.f;
	}
	ASSERT_NOT_REACHED();
	return 0.0f;
}

const struct valueNode *newVecToValue(const struct node_storage *s, const struct vectorNode *vec, enum cr_vec_to_value_component component) {
	HASH_CONS(s->node_table, hash, struct vecToValueNode, {
		.vec = vec ? vec : newConstantVector(s, vec_zero()),
		.component_to_get = component,
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
