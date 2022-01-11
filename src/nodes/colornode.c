//
//  texturenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/color.h"
#include "../renderer/samplers/sampler.h"
#include "../datatypes/vector.h"
#include "../datatypes/material.h"
#include "../datatypes/poly.h"
#include "bsdfnode.h"

#include "colornode.h"

const struct colorNode *unknownTextureNode(const struct world *w) {
	return newConstantTexture(w, blackColor);
}
