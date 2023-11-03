//
//  pathtrace.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/vector.h"
#include "../datatypes/poly.h"
#include "../datatypes/lightray.h"
#include "../datatypes/material.h"
#include "../nodes/bsdfnode.h"

struct world;

struct color path_trace(struct lightRay incident, const struct world *scene, int max_bounces, sampler *sampler);
