//
//  light_path.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 21/12/2020.
//  Copyright © 2020-2024 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>

const struct valueNode *new_light_path(const struct node_storage *s, enum cr_light_path_info query);
