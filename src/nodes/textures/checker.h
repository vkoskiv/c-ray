//
//  checker.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

struct textureNode *newColorCheckerBoardTexture(struct world *world, struct textureNode *colorA, struct textureNode *colorB, float size);

struct textureNode *newCheckerBoardTexture(struct world *world, float size);

struct textureNode *newCheckerBoardTexture(struct world *world, float size);
