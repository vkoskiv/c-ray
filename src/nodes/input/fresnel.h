//
//  fresnel.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vectorNode;

struct valueNode *newFresnel(const struct world *world, struct valueNode *IOR, struct vectorNode *normal);
