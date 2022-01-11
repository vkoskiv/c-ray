//
//  bsdfnode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/vector.h"
#include "../datatypes/color.h"
#include "../datatypes/material.h"
#include "bsdfnode.h"

const struct bsdfNode *warningBsdf(const struct world *world) {
	return newMix(world,
				  newDiffuse(world, newConstantTexture(world, warningMaterial().diffuse)),
				  newDiffuse(world, newConstantTexture(world, (struct color){0.2f, 0.2f, 0.2f, 1.0f})),
				  newGrayscaleConverter(world, newCheckerBoardTexture(world, NULL, NULL, newConstantValue(world, 500.0f))));
}
