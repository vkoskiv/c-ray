//
//  valuenode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "nodebase.h"

struct valueNode {
	struct nodeBase base;
	float (*eval)(const struct valueNode *node, const struct hitRecord *record);
};

#include "input/fresnel.h"

struct valueNode *newConstantValue(const struct world *world, float value);
