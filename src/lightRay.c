//
//  lightRay.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2018 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "lightRay.h"

struct lightRay newRay(struct vector start, struct vector direction, enum type rayType) {
	return (struct lightRay){start, direction, rayType, {0}, 0};
}
