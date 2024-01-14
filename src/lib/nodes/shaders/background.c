//
//  background.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/color.h"
#include "../../../common/vector.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../bsdfnode.h"

#include "background.h"

struct backgroundBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
	const struct valueNode *strength;
	const struct vectorNode *pose;
	bool blender;
};

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

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct backgroundBsdf *self = (struct backgroundBsdf *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	char strength[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	if (self->strength->base.dump) self->strength->base.dump(self->strength, strength, sizeof(strength));
	snprintf(dumpbuf, bufsize, "backgroundBsdf { color: %s, strength: %s }", color, strength);
}

static inline void recompute_uv(struct hitRecord *isect, float offset, bool blender) {
	struct vector ud = vec_normalize(isect->incident_dir);
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi;
	float theta;
	if (blender) {
		phi = (atan2f(ud.y, -ud.x) / 4.0f) + offset;
		theta = acosf((-ud.z / r));
	} else {
		phi = (atan2f(ud.z, ud.x) / 4.0f) + offset;
		theta = acosf((-ud.y / r));
	}

	float u = (phi / (PI / 2.0f));
	float v = theta / PI;

	u = wrap_min_max(u, 0.0f, 1.0f);
	v = wrap_min_max(v, 0.0f, 1.0f);

	isect->uv = (struct coord){ u, v };
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	(void)sampler;
	struct backgroundBsdf *background = (struct backgroundBsdf *)bsdf;
	float strength = background->strength->eval(background->strength, sampler, record);
	float pose = background->pose->eval(background->pose, sampler, record).f;
	pose = deg_to_rad(pose) / 4.0f;
	struct hitRecord copy = *record;
	recompute_uv(&copy, pose, background->blender);
	return (struct bsdfSample){
		.out = { { 0 } },
		.weight = colorCoef(strength, background->color->eval(background->color, sampler, &copy))
	};
}

const struct bsdfNode *newBackground(const struct node_storage *s, const struct colorNode *tex, const struct valueNode *strength, const struct vectorNode *pose, bool blender) {
	HASH_CONS(s->node_table, hash, struct backgroundBsdf, {
		.color = tex ? tex : newConstantTexture(s, g_gray_color),
		.strength = strength ? strength : newConstantValue(s, 1.0f),
		.pose = pose ? pose : newConstantVector(s, (struct vector){ 0 }),
		.blender = blender,
		.bsdf = {
			.sample = sample,
			.base = { .compare = compare, .dump = dump }
		}
	});
}
