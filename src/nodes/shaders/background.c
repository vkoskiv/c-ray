//
//  background.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/color.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/hitrecord.h"
#include "../../utils/hashtable.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "background.h"

struct backgroundBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *strength;
	const struct valueNode *offset;
};

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct backgroundBsdf *background = (struct backgroundBsdf *)bsdf;
	// We need to inject the new UV using spherical mapping here.
	//TODO: Find a better way to do this.
	//Ideally it would be populated by the renderer before we eval bsdfs.
	struct hitRecord *copy = (struct hitRecord *)record; // Oof owie, my const...
	struct vector ud = vecNormalize(copy->incident.direction);
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x) / 4.0f) + background->offset->eval(background->offset, record);
	float theta = acosf((-ud.y / r));
	
	float u = theta / PI;
	float v = (phi / (PI / 2.0f));
	
	u = wrapMinMax(u, 0.0f, 1.0f);
	v = wrapMinMax(v, 0.0f, 1.0f);
	
	copy->uv = (struct coord){ v, u };
	
	float strength = background->strength->eval(background->strength, record);
	
	return (struct bsdfSample){
		.out = vecZero(),
		.color = colorCoef(strength, background->color->eval(background->color, copy))
	};
}

static bool compare(const void *A, const void *B) {
	const struct backgroundBsdf *this = A;
	const struct backgroundBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct backgroundBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

const struct bsdfNode *newBackground(const struct world *world, const struct colorNode *tex, const struct valueNode *strength, const struct valueNode *offset) {
	HASH_CONS(world->nodeTable, &world->nodePool, hash, struct backgroundBsdf, {
		.color = tex ? tex : newConstantTexture(world, grayColor),
		.strength = strength ? strength : newConstantValue(world, 1.0f),
		.offset = offset ? offset : newConstantValue(world, 0.0f),
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare }
		}
	});
}
