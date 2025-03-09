//
//  fresnel.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/12/2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vectorNode;

const struct valueNode *newFresnel(const struct node_storage *s, const struct valueNode *IOR, const struct vectorNode *normal);
