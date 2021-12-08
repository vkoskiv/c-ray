//
//  sceneloader.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../libraries/cJSON.h"

struct renderer;

int parseJSON(struct renderer *r, char *input);
