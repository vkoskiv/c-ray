//
//  fresnel.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
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
	float cosine = 0.0f;
	if (vecDot(record->incident.direction, record->surfaceNormal) > 0.0f) {
		cosine = IOR * vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction);
	} else {
		cosine = -(vecDot(record->incident.direction, record->surfaceNormal) / vecLength(record->incident.direction));
	}
	
	return schlick(cosine, this->IOR->eval(this->IOR, record));
}

const struct valueNode *newFresnel(const struct world *world, const struct valueNode *IOR, const struct vectorNode *normal) {
	HASH_CONS(world->nodeTable, world->nodePool, hash, struct fresnelNode, {
		.IOR = IOR,
		.normal = normal,
		.node = {
			.eval = eval,
			.base = { .compare = compare }
		}
	});
}
