//
//  valuenode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../vendored/cJSON.h"
#include "nodebase.h"

struct hitRecord;

struct valueNode {
	struct nodeBase base;
	float (*eval)(const struct valueNode *node, const struct hitRecord *record);
};

#include "input/fresnel.h"
#include "input/raylength.h"
#include "textures/alpha.h"
#include "converter/math.h"
#include "converter/map_range.h"
#include "converter/vectovalue.h"

const struct valueNode *newConstantValue(const struct node_storage *s, float value);

struct file_cache;
const struct valueNode *parseValueNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node);
