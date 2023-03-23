//
//  fresnel.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../nodebase.h"
#include "../../datatypes/scene.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../valuenode.h"
#include "../vectornode.h"

#include "fresnel.h"

struct fresnelNode {
	struct valueNode node;
	const struct valueNode *IOR;
	const struct vectorNode *normal;
};

static bool compare(const void *A, const void *B) {
	const struct fresnelNode *this = A;
	const struct fresnelNode *other = B;
	return this->IOR == other->IOR && this->normal == other->normal;
}

static uint32_t hash(const void *p) {
	const struct fresnelNode *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->IOR, sizeof(this->IOR));
	h = hashBytes(h, &this->normal, sizeof(this->normal));
	return h;
}

static void dump(const void *node, char *dumpbuf) {
	struct fresnelNode *self = (struct fresnelNode *)node;
	char ior[DUMPBUF_SIZE / 4] = "";
	char normal[DUMPBUF_SIZE / 4] = "";
	if (self->IOR->base.dump) self->IOR->base.dump(self->IOR, &ior[0]);
	if (self->normal->base.dump) self->normal->base.dump(self->normal, &normal[0]);
	snprintf(dumpbuf, DUMPBUF_SIZE, "fresnelNode { IOR: %s, normal: %s }", ior, normal);
}

static float eval(const struct valueNode *node, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct fresnelNode *this = (struct fresnelNode *)node;
	
	float IOR = this->IOR->eval(this->IOR, sampler, record);
	float cosine;
	if (vec_dot(record->incident_dir, record->surfaceNormal) > 0.0f) {
		cosine = IOR * vec_dot(record->incident_dir, record->surfaceNormal) / vec_length(record->incident_dir);
	} else {
		cosine = -(vec_dot(record->incident_dir, record->surfaceNormal) / vec_length(record->incident_dir));
	}
	
	return schlick(cosine, this->IOR->eval(this->IOR, sampler, record));
}

const struct valueNode *newFresnel(const struct node_storage *s, const struct valueNode *IOR, const struct vectorNode *normal) {
	HASH_CONS(s->node_table, hash, struct fresnelNode, {
		.IOR = IOR ? IOR : newConstantValue(s, 1.45f),
		.normal = normal ? normal : newNormal(s),
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
