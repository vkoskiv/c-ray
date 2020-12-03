//
//  metal.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct bsdf *newMetal(struct textureNode *color, struct textureNode *roughness);

void destroyMetal(struct bsdf *bsdf);
