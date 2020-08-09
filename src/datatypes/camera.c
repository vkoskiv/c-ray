//
//  camera.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "transforms.h"
#include "camera.h"

#include "vector.h"

void updateCam(struct camera *cam) {
	cam->forward = vecNormalize(cam->lookAt);
	cam->right = vecCross(worldUp, cam->forward);
	cam->up = vecCross(cam->forward, cam->right);
}

struct camera *newCamera(int width, int height) {
	struct camera *cam = calloc(1, sizeof(*cam));
	cam->width = width;
	cam->height = height;
	updateCam(cam);
	return cam;
}

void camSetLookAt(struct camera *cam, struct vector lookAt) {
	cam->lookAt = lookAt;
	updateCam(cam);
}

struct lightRay getCameraRay(struct camera *cam, int x, int y) {
	struct lightRay newRay = {0};
	
	newRay.start = vecZero();
	
	float aspectRatio = cam->width / cam->height;
	float sensorWidth = 2.0f * tanf(toRadians(cam->FOV) / 2.0f);
	float sensorHeight = sensorWidth / aspectRatio;
	
	camSetLookAt(cam, (struct vector){0.0f, 0.0f, 1.0f});
	
	struct vector pixX = vecScale(cam->right, (sensorWidth / cam->width));
	struct vector pixY = vecScale(cam->up, (sensorHeight / cam->height));
	struct vector pixV = vecAdd(
							cam->forward,
							vecAdd(
								vecScale(pixX, x - cam->width * 0.5f),
								vecScale(pixY, y - cam->height * 0.5f)
							)
						);
	
	newRay.direction = vecNormalize(pixV);
	
	//To world space
	transformRay(&newRay, &cam->composite.A);
	return newRay;
}

void destroyCamera(struct camera *cam) {
	if (cam) {
		free(cam);
	}
}
