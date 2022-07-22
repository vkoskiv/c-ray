//
//  alpha.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 23/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct colorNode;

const struct valueNode *newAlpha(const struct node_storage *s, const struct colorNode *color);
