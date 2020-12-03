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

void destroyBsdf(struct bsdf *bsdf) {
	bsdf->destroy(bsdf);
}
