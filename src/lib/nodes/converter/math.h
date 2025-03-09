//
//  math.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>

const struct valueNode *newMath(const struct node_storage *s, const struct valueNode *A, const struct valueNode *B, const enum cr_math_op op);

