//
//  camera.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 02/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "camera.h"

//find vectorLength and see if normalize could be used there!

void calculateUVW(camera *camera) {
	vector t = subtractVectors(&camera->lookAt, &camera->pos);
	camera->w = normalizeVector(&t);
	vector t1 = vectorCross(&camera->up, &camera->w);
	camera->u = normalizeVector(&t1);
	vector t2 = vectorCross(&camera->w, &camera->u);
	camera->v = t2;
}
