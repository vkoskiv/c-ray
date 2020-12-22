//
//  background.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 19/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct bsdfNode *newBackground(const struct world *world, const struct colorNode *tex, const struct valueNode *strength, const struct valueNode *offset);
