//
//  vecmath.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>

const struct vectorNode *newVecMath(const struct node_storage *s, const struct vectorNode *A, const struct vectorNode *B, const struct vectorNode *C, const struct valueNode *f, const enum cr_vec_op op);
