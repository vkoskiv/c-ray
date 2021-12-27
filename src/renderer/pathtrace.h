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

/// Iterative path tracer.
/// @param incidentRay View ray to be casted into the scene
/// @param scene Scene to cast the ray into
/// @param maxDepth Maximum depth of path
/// @param rng A random number generator. One per execution thread.
struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler);
