//
//  bsdf.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/color.h"
#include "../samplers/sampler.h"
#include "../../datatypes/material.h"
#include "bsdf.h"

#include "diffuse.h"
#include "glass.h"
#include "metal.h"

struct bsdf *newBsdf(enum bsdfType type) {
	switch (type) {
		case lambertian:
			return (struct bsdf*)newDiffuse();
			break;
		case glass:
			return (struct bsdf*)newGlass();
			break;
		case metal:
			return (struct bsdf*)newMetal();
			break;
		default:
			return (struct bsdf*)newDiffuse();
			break;
	}
}
