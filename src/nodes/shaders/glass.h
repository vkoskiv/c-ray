//
//  glass.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct bsdf *newGlass(struct world *world, struct textureNode *color, struct textureNode *roughness, float IOR);
