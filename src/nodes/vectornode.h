//
//  vectornode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../vendored/cJSON.h"
#include "../renderer/samplers/sampler.h"
#include "../datatypes/vector.h"
#include "nodebase.h"

struct vectorValue {
	union {
		struct vector v;
		struct coord c;
		float f;
	};
};

struct vectorNode {
	struct nodeBase base;
	struct vectorValue (*eval)(const struct vectorNode *node, sampler *sampler, const struct hitRecord *record);
};

#include "input/normal.h"
#include "input/uv.h"
#include "converter/vecmath.h"
#include "converter/vectocolor.h"
#include "converter/vecmix.h"

const struct vectorNode *newConstantVector(const struct node_storage *storage, struct vector vector);
const struct vectorNode *newConstantUV(const struct node_storage *s, const struct coord c);

const struct vectorNode *build_vector_node(struct cr_renderer *r_ext, const struct cr_vector_node *desc);
