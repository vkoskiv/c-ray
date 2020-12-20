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
	
	int width;
	int height;
};

struct camera *newCamera(unsigned width, unsigned height, float FOV, float focalDistance, float fstops, struct transform composite);

struct lightRay getCameraRay(struct camera *cam, int x, int y, struct sampler *sampler);

void destroyCamera(struct camera *cam);
