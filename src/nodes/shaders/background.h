//
//  background.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct bsdfNode *newBackground(struct world *world, struct colorNode *tex, struct valueNode *strength, float offset);
