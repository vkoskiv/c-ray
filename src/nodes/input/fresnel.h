//
//  fresnel.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vectorNode;

const struct valueNode *newFresnel(const struct world *world, const struct valueNode *IOR, const struct vectorNode *normal);
