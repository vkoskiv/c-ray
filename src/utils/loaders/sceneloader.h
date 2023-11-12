//
//  sceneloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../vendored/cJSON.h"

struct cr_renderer;

int parse_json(struct cr_renderer *r, cJSON *json);
