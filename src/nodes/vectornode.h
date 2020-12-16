//
//  vectornode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "nodebase.h"

struct vectorNode {
	struct nodeBase base;
	struct vector (*eval)(const struct vectorNode *node);
};
