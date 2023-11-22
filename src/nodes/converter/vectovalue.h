//
//  vectovalue.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>

const struct valueNode *newVecToValue(const struct node_storage *s, const struct vectorNode *vec, enum cr_vec_to_value_component component);
