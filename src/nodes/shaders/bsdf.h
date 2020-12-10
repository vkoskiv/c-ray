//
//  bsdf.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../datatypes/lightRay.h"
#include "../../utils/mempool.h"
#include "../textures/texturenode.h"
#include "../../datatypes/hitrecord.h"
#include "../nodebase.h"

struct bsdfSample {
	struct vector out;
	float pdf;
	struct color color;
};

//TODO: Expand and refactor to match a standard bsdf signature with eval, sample and pdf
struct bsdf {
	struct nodeBase base;
	struct bsdfSample (*sample)(const struct bsdf *bsdf, sampler *sampler, const struct hitRecord *record);
};

#include "diffuse.h"
#include "glass.h"
#include "metal.h"
#include "mix.h"
#include "plastic.h"

struct bsdf *warningBsdf(struct world *world);
