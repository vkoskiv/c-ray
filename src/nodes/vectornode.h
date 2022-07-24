//
//  vectornode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../vendored/cJSON.h"
#include "nodebase.h"

struct vectorValue {
	struct vector v;
	struct coord c;
	float f;
};

struct vectorNode {
	struct nodeBase base;
	struct vectorValue (*eval)(const struct vectorNode *node, const struct hitRecord *record);
};

#include "input/normal.h"
#include "input/uv.h"
#include "converter/vecmath.h"
#include "converter/vectocolor.h"

const struct vectorNode *newConstantVector(const struct node_storage *storage, struct vector vector);
const struct vectorNode *parseVectorNode(const struct node_storage *s, const struct cJSON *node);