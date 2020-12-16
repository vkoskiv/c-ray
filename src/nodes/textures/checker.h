//
//  checker.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

struct colorNode *newColorCheckerBoardTexture(struct world *world, struct colorNode *colorA, struct colorNode *colorB, float size);

struct colorNode *newCheckerBoardTexture(struct world *world, float size);

struct colorNode *newCheckerBoardTexture(struct world *world, float size);
