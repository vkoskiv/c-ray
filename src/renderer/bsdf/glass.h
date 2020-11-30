//
//  glass.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct glassBsdf {
	struct bsdf bsdf;
	struct textureNode *roughness;
	struct textureNode *color;
};

struct glassBsdf *newGlass(struct textureNode *color, struct textureNode *roughness);
