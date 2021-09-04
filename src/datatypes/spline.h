//
//  spline.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

struct spline;

struct spline *spline_new(const struct vector a, const struct vector b, const struct vector c, const struct vector d);
struct vector spline_at(const struct spline *s, const float t);
void spline_destroy(struct spline *);

