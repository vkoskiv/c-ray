//
//  camera.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "camera.h"

#include "scene.h"
#include "../datatypes/texture.h"

/**
 Compute view direction transforms
 
 @param direction Direction vector to be transformed
 */
void transformCameraView(struct camera *cam, struct vector *direction) {
	for (int i = 1; i < cam->transformCount; i++) {
		transformVector(direction, &cam->transforms[i]);
	}
}

//FIXME: Move image to camera and fix this
void computeFocalLength(struct camera *camera, int width) {
	if (camera->FOV > 0.0 && camera->FOV < 189.0) {
		camera->focalLength = 0.5 * width / toRadians(0.5 * camera->FOV);
	}
}

void initCamera(struct camera *cam) {
	cam->pos = vecWithPos(0, 0, 0);
	cam->up = vecWithPos(0, 1, 0);
	cam->left = vecWithPos(-1, 0, 0);
}

//TODO: Fix so the translate transform is always performed correctly no matter what order transforms are given in
void transformCameraIntoView(struct camera *cam) {
	//Compute transforms for position (place the camera in the scene)
	transformVector(&cam->pos, &cam->transforms[0]);
	
	//...and compute rotation transforms for camera orientation (point the camera)
	transformCameraView(cam, &cam->left);
	transformCameraView(cam, &cam->up);
}

void freeCamera(struct camera *cam) {
	if (cam->transforms) {
		free(cam->transforms);
	}
}
