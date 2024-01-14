//
//  translucent.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 05/06/2022.
//  Copyright © 2022-2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../renderer/samplers/sampler.h"
#include "../../renderer/samplers/vec.h"
#include "../../../common/color.h"
#include "../../../common/vector.h"
#include "../../../common/hashtable.h"
#include "../../datatypes/hitrecord.h"
#include "../../datatypes/scene.h"
#include "../colornode.h"
#include "../bsdfnode.h"

#include "translucent.h"

struct translucentBsdf {
	struct bsdfNode bsdf;
	const struct colorNode *color;
};

static bool compare(const void *A, const void *B) {
	const struct translucentBsdf *this = A;
	const struct translucentBsdf *other = B;
	return this->color == other->color;
}

static uint32_t hash(const void *p) {
	const struct translucentBsdf *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->color, sizeof(this->color));
	return h;
}

static void dump(const void *node, char *dumpbuf, int bufsize) {
	struct translucentBsdf *self = (struct translucentBsdf *)node;
	char color[DUMPBUF_SIZE / 2] = "";
	if (self->color->base.dump) self->color->base.dump(self->color, color, sizeof(color));
	snprintf(dumpbuf, bufsize, "translucentBsdf { color: %s }", color);
}

static struct bsdfSample sample(const struct bsdfNode *bsdf, sampler *sampler, const struct hitRecord *record) {
	struct translucentBsdf *diffBsdf = (struct translucentBsdf *)bsdf;
	const struct vector scatterDir = vec_normalize(vec_add(vec_negate(record->surfaceNormal), vec_on_unit_sphere(sampler)));
	return (struct bsdfSample){
			.out = { .start = record->hitPoint, .direction = scatterDir, .type = rt_transmission | rt_diffuse },
			.weight = diffBsdf->color->eval(diffBsdf->color, sampler, record)
	};
}

const struct bsdfNode *newTranslucent(const struct node_storage *s, const struct colorNode *color) {
	HASH_CONS(s->node_table, hash, struct translucentBsdf, {
		.color = color ? color : newConstantTexture(s, g_black_color),
		.bsdf = {
				.sample = sample,
				.base = { .compare = compare, .dump = dump }
		}
	});
}
