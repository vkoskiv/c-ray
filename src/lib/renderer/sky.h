//
//  sky.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/06/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct color;
struct lightRay;

// This models atmospheric rayleigh scattering to produce
// a realistic looking sky up in the +Y direction.
// This is not very useful at the moment, since our
// path tracer does not do any kind of direct sampling,
// meaning our render will have a ton of noise.
// A lot of the physical parameters are tweakable in the
// start of the implementation file.
struct color sky(struct lightRay incidentRay);
