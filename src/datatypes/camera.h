//
//  camera.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "transforms.h"
#include "vector.h"
#include "lightray.h"
#include "spline.h"

struct not_a_quaternion {
	float rotX;
	float rotY;
	float rotZ;
};

struct camera {
	float FOV;
	float focalLength;
	float focalDistance;
	float fstops;
	float aperture;
	float aspectRatio;
	struct coord sensorSize;
	
	struct vector up;
	struct vector right;
	struct vector lookAt;
	struct vector forward;
	
	struct transform composite;
	struct not_a_quaternion orientation;
	struct vector position;
	
	struct spline *path;
	float time;
	
	int width;
	int height;
};

struct camera *camNew(unsigned width, unsigned height, float FOV, float focalDistance, float fstops);
void camUpdate(struct camera *cam, const struct not_a_quaternion *orientation, const struct vector *pos);
struct lightRay camGetRay(struct camera *cam, int x, int y, struct sampler *sampler);
void camDestroy(struct camera *cam);
