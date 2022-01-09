//
//  spline.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/09/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

struct spline;

struct spline *spline_new(struct vector a, struct vector b, struct vector c, struct vector d);
struct vector spline_at(const struct spline *s, const float t);
void spline_destroy(struct spline *);

