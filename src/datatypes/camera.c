//
//  camera.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "camera.h"

#include "../utils/filehandler.h"
#include "../renderer/renderer.h"
#include "scene.h"

/**
 Compute view direction transforms
 
 @param direction Direction vector to be transformed
 */
void transformCameraView(struct camera *cam, struct vector *direction) {
	for (int i = 1; i < cam->transformCount; i++) {
		transformVector(direction, &cam->transforms[i]);
		direction->isTransformed = false;
	}
}

void computeFocalLength(struct renderer *renderer) {
	if (renderer->scene->camera->FOV > 0.0 && renderer->scene->camera->FOV < 189.0) {
		renderer->scene->camera->focalLength = 0.5 * *renderer->image->width / toRadians(0.5 * renderer->scene->camera->FOV);
	}
}

void initCamera(struct camera *cam) {
	cam->pos = vectorWithPos(0, 0, 0);
	cam->up = vectorWithPos(0, 1, 0);
	cam->left = vectorWithPos(-1, 0, 0);
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
