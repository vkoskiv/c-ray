//
//  color_ramp.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 12/01/2024.
//  Copyright © 2024-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <common/color.h>
#include <common/dyn_array.h>
#include "../valuenode.h"

const struct colorNode *new_color_ramp(const struct node_storage *s,
                                       const struct valueNode *input_value,
                                       enum cr_color_mode color_mode,
                                       enum cr_interpolation interpolation,
                                       struct ramp_element *elements,
                                       int element_count);

