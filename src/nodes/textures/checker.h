//
//  checker.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

const struct colorNode *newColorCheckerBoardTexture(const struct world *world, const struct colorNode *colorA, const struct colorNode *colorB, float size);

const struct colorNode *newCheckerBoardTexture(const struct world *world, float size);

const struct colorNode *newCheckerBoardTexture(const struct world *world, float size);
