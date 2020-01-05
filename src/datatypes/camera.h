//
//  camera.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"

struct camera {
	float FOV;
	float focalLength;
	float aperture;
	
	struct vector pos;
	struct vector up;
	struct vector left;
	
	struct transform *transforms;
	int transformCount;
};

//Compute focal length for camera
void computeFocalLength(struct camera *camera, int width);
void initCamera(struct camera *cam);
void transformCameraView(struct camera *cam, struct vector *direction); //For transforming direction in renderer
void transformCameraIntoView(struct camera *cam); //Run once in scene.c to calculate pos, up, left

void freeCamera(struct camera *cam);
