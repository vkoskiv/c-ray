//
//  pathtrace.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../datatypes/vector.h"
#include "../datatypes/poly.h"
#include "../datatypes/lightRay.h"
#include "../datatypes/material.h"
#include "bsdf/bsdf.h"

struct world;

/// Recursive path tracer.
/// @param incidentRay View ray to be casted into the scene
/// @param scene Scene to cast the ray into
/// @param maxDepth Maximum depth of recursion
/// @param rng A random number generator. One per execution thread.
struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler);
