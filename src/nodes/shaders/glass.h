//
//  glass.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct bsdfNode *newGlass(const struct node_storage *s, const struct colorNode *color, const struct valueNode *roughness, const struct valueNode *IOR);
