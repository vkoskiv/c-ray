//
//  valuenode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../renderer/samplers/sampler.h"
#include "nodebase.h"

struct hitRecord;

struct valueNode {
	struct nodeBase base;
	float (*eval)(const struct valueNode *node, sampler *sampler, const struct hitRecord *record);
	bool constant;
};

#include "input/fresnel.h"
#include "input/raylength.h"
#include "textures/alpha.h"
#include "converter/math.h"
#include "converter/map_range.h"
#include "converter/vectovalue.h"

const struct valueNode *newConstantValue(const struct node_storage *s, float value);

const struct valueNode *build_value_node(struct cr_scene *s_ext, const struct cr_value_node *desc);
