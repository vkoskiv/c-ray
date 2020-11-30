//
//  mix.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct mixBsdf {
	struct bsdf bsdf;
	struct bsdf *A;
	struct bsdf *B;
	struct textureNode *lerp;
};

struct mixBsdf *newMix(struct bsdf *A, struct bsdf *B, struct textureNode *lerp);
