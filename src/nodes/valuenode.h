//
//  valuenode.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 16/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct valueNode {
	struct nodeBase base;
	float (*eval)(const struct valueNode *node);
};
