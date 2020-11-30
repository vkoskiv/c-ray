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
	struct color (*eval)(const struct hitRecord*);
};

struct mixBsdf *newMix(void);
