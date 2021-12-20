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
#include "../renderer/samplers/sampler.h"

void cam_recompute_optics(struct camera *cam) {
	if (!cam) return;
	cam->aspectRatio = (float)cam->width / (float)cam->height;
	cam->sensorSize.x = 2.0f * tanf(toRadians(cam->FOV) / 2.0f);
	cam->sensorSize.y = cam->sensorSize.x / cam->aspectRatio;
	cam->lookAt = (struct vector){0.0f, 0.0f, 1.0f};
	//FIXME: This still assumes a 35mm sensor, instead of using the computed
	//sensor width value from above. Just preserving this so existing configs
	//work, but do look into a better way to do this here!
	const float sensor_width_35mm = 0.036f;
	cam->focalLength = 0.5f * sensor_width_35mm / toRadians(0.5f * cam->FOV);
	if (cam->fstops != 0.0f) cam->aperture = 0.5f * (cam->focalLength / cam->fstops);
	cam->forward = vecNormalize(cam->lookAt);
	cam->right = vecCross(worldUp, cam->forward);
	cam->up = vecCross(cam->forward, cam->right);
}

void recomputeComposite(struct camera *cam) {
	struct transform transforms[2];
	if (cam->path) {
		struct vector positionAtT = spline_at(cam->path, cam->time);
		transforms[0] = newTransformTranslate(positionAtT.x, positionAtT.y, positionAtT.z);
	} else {
		transforms[0] = newTransformTranslate(cam->position.x, cam->position.y, cam->position.z);
	}
	transforms[1] = newTransformRotate(cam->orientation.roll, cam->orientation.pitch, cam->orientation.yaw);

	struct transform composite = { .A = identityMatrix() };
	for (int i = 0; i < 2; ++i) {
		composite.A = multiplyMatrices(composite.A.mtx, transforms[i].A.mtx);
	}
	
	composite.Ainv = inverseMatrix(composite.A.mtx);
	composite.type = transformTypeComposite;
	cam->composite = composite;
}

void cam_update_pose(struct camera *cam, const struct euler_angles *orientation, const struct vector *pos) {
	if (orientation) cam->orientation = *orientation;
	if (pos) cam->position = *pos;
	if (orientation || pos) recomputeComposite(cam);
}

static inline float sign(float v) {
	return (v >= 0.0f) ? 1.0f : -1.0f;
}

// Convert uniform distribution into triangle-shaped distribution
// From https://www.shadertoy.com/view/4t2SDh
static inline float triangleDistribution(float v) {
	const float orig = v * 2.0f - 1.0f;
	v = orig / sqrtf(fabsf(orig));
	v = clamp(v, -1.0f, 1.0f); // Clamping it like this might be a bit overkill.
	v = v - sign(orig);
	return v;
}

struct lightRay camGetRay(struct camera *cam, int x, int y, struct sampler *sampler) {
	struct lightRay newRay = {{0}};
	
	newRay.start = vecZero();
	
	const float jitterX = triangleDistribution(getDimension(sampler));
	const float jitterY = triangleDistribution(getDimension(sampler));
	
	struct vector pixX = vecScale(cam->right, (cam->sensorSize.x / cam->width));
	struct vector pixY = vecScale(cam->up, (cam->sensorSize.y / cam->height));
	struct vector pixV = vecAdd(
							cam->forward,
							vecAdd(
								vecScale(pixX, x - cam->width * 0.5f + jitterX + 0.5f),
								vecScale(pixY, y - cam->height * 0.5f + jitterY + 0.5f)
							)
						);
	newRay.direction = vecNormalize(pixV);
	
	if (cam->aperture > 0.0f) {
		float ft = cam->focalDistance / vecDot(newRay.direction, cam->forward);
		struct vector focusPoint = alongRay(&newRay, ft);
		struct coord lensPoint = coordScale(cam->aperture, randomCoordOnUnitDisc(sampler));
		newRay.start = vecAdd(newRay.start, vecAdd(vecScale(cam->right, lensPoint.x), vecScale(cam->up, lensPoint.y)));
		newRay.direction = vecNormalize(vecSub(focusPoint, newRay.start));
	}
	//To world space
	transformRay(&newRay, cam->composite.A.mtx);
	return newRay;
}

void camDestroy(struct camera *cam) {
	if (cam) {
		free(cam);
	}
}
