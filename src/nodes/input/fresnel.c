//
//  fresnel.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

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

static float eval(const struct valueNode *node, const struct hitRecord *record) {
	struct fresnelNode *this = (struct fresnelNode *)node;
	
	float IOR = this->IOR->eval(this->IOR, record);
	float cosine;
	if (vecDot(record->incident_dir, record->surfaceNormal) > 0.0f) {
		cosine = IOR * vecDot(record->incident_dir, record->surfaceNormal) / vecLength(record->incident_dir);
	} else {
		cosine = -(vecDot(record->incident_dir, record->surfaceNormal) / vecLength(record->incident_dir));
	}
	
	return schlick(cosine, this->IOR->eval(this->IOR, record));
}

const struct valueNode *newFresnel(const struct node_storage *s, const struct valueNode *IOR, const struct vectorNode *normal) {
	HASH_CONS(s->node_table, hash, struct fresnelNode, {
		.IOR = IOR ? IOR : newConstantValue(s, 1.45f),
		.normal = normal ? normal : newNormal(s),
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
