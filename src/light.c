//
//  light.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 11/10/15.
//  Copyright © 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "light.h"

struct light newLight(struct vector pos, double power, struct color diffuse) {
	return (struct light){pos, power, diffuse};
}
