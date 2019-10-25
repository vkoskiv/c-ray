//
//  lightRay.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "lightRay.h"

struct lightRay newRay(vec3 start, vec3 direction, enum type rayType) {
	return (struct lightRay){start, direction, rayType, NewMaterial(MATERIAL_TYPE_DEFAULT), rayTypeIncident};
}
