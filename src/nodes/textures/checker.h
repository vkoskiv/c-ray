//
//  checker.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct textureNode *newColorCheckerBoardTexture(struct block **pool, struct textureNode *colorA, struct textureNode *colorB, float size);

struct textureNode *newCheckerBoardTexture(struct block **pool, float size);struct textureNode *newColorCheckerBoardTexture(struct block **pool, struct textureNode *colorA, struct textureNode *colorB, float size);

struct textureNode *newCheckerBoardTexture(struct block **pool, float size);
