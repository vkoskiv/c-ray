//
//  vectornode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "nodebase.h"

struct vectorNode {
	struct nodeBase base;
	struct vector (*eval)(const struct vectorNode *node, const struct hitRecord *record);
};

struct vectorNode *newConstantVector(const struct world *world, struct vector vector);
