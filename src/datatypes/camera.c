//
//  camera.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "camera.h"

#include "scene.h"
#include "texture.h"
#include "vector.h"
#include "transforms.h"

/**
 Compute view direction transforms
 
 @param direction Direction vector to be transformed
 */
void transformCameraView(struct camera *cam, struct vector *direction) {
	for (int i = 1; i < cam->transformCount; i++) {
		transformDirection(direction, cam->transforms[i].A);
	}
}

//FIXME: Move image to camera and fix this
void computeFocalLength(struct camera *camera, int width) {
	// aperture = 0.5 * (focalLength / fstops)
	if (camera->FOV > 0.0f && camera->FOV < 189.0f) {
		camera->focalLength = 0.5f * width / toRadians(0.5f * camera->FOV);
	}
	
	//FIXME: This assumes a 35mm sensor, which we aren't really dealing with most of the time.
	//TODO: Properly identify sensor dimensions and compute this in a more reasonable way.
	float w = 0.036f;
	float flenght = 0.5f * w / toRadians(0.5f * camera->FOV);
	//Recompute aperture based on fstops
	if (camera->fstops != 0.0f) camera->aperture = 0.5f * (flenght / camera->fstops);
	return;
}

float acomputeFocalLength(float FOV, unsigned width) {
	if (FOV > 0.0f && FOV < 189.0f) {
		return 0.5f * width / toRadians(0.5f * FOV);
	}
	return 0.5f * width / toRadians(0.5f * 80.0f);
}

void initCamera(struct camera *cam) {
	if (!cam->transforms) cam->transforms = calloc(1, sizeof(struct transform));
	cam->pos =  vecWithPos(0.0f, 0.0f, 0.0f);
	cam->up =   vecWithPos(0.0f, 1.0f, 0.0f);
	cam->left = vecWithPos(-1.0f, 0.0f, 0.0f);
}

//TODO: Fix so the translate transform is always performed correctly no matter what order transforms are given in
void transformCameraIntoView(struct camera *cam) {
	initCamera(cam);
	//Compute transforms for position (place the camera in the scene)
	transformVector(&cam->pos, cam->transforms[0].A);
	
	//...and compute rotation transforms for camera orientation (point the camera)
	transformCameraView(cam, &cam->left);
	transformCameraView(cam, &cam->up);
}

void destroyCamera(struct camera *cam) {
	if (cam->transforms) {
		free(cam->transforms);
		free(cam);
	}
}
