//
//  camera.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "transforms.h"
#include "camera.h"

#include "vector.h"

void cam_recompute_optics(struct camera *cam) {
	if (!cam) return;
	cam->aspect_ratio = (float)cam->width / (float)cam->height;
	cam->sensor_size.x = 2.0f * tanf(toRadians(cam->FOV) / 2.0f);
	cam->sensor_size.y = cam->sensor_size.x / cam->aspect_ratio;
	cam->look_at = (struct vector){0.0f, 0.0f, 1.0f};
	//FIXME: This still assumes a 35mm sensor, instead of using the computed
	//sensor width value from above. Just preserving this so existing configs
	//work, but do look into a better way to do this here!
	const float sensor_width_35mm = 0.036f;
	cam->focal_length = 0.5f * sensor_width_35mm / toRadians(0.5f * cam->FOV);
	if (cam->fstops != 0.0f) cam->aperture = 0.5f * (cam->focal_length / cam->fstops);
	cam->forward = vecNormalize(cam->look_at);
	cam->right = vecCross(worldUp, cam->forward);
	cam->up = vecCross(cam->forward, cam->right);
}

void recomputeComposite(struct camera *cam) {
	struct transform transforms[2];
	if (cam->path) {
		struct vector positionAtT = spline_at(cam->path, cam->time);
		transforms[0] = newTransformTranslate(positionAtT.x, positionAtT.y, positionAtT.z);
	} else {
		transforms[0] = newTransformTranslate(-cam->position.x, cam->position.y, cam->position.z);
	}
	transforms[1] = newTransformRotate(cam->orientation.roll, cam->orientation.pitch, cam->orientation.yaw);

	struct transform composite = { .A = identityMatrix() };
	for (int i = 0; i < 2; ++i) {
		composite.A = multiplyMatrices(composite.A, transforms[i].A);
	}
	
	composite.Ainv = inverseMatrix(composite.A);
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

struct lightRay cam_get_ray(const struct camera *cam, int x, int y, struct sampler *sampler) {
	struct lightRay new_ray = {{0}};
	
	new_ray.start = vecZero();
	
	const float jitter_x = triangleDistribution(getDimension(sampler));
	const float jitter_y = triangleDistribution(getDimension(sampler));
	
	const struct vector pix_x = vecScale(vecNegate(cam->right), (cam->sensor_size.x / cam->width));
	const struct vector pix_y = vecScale(cam->up, (cam->sensor_size.y / cam->height));
	const struct vector pix_v = vecAdd(
							cam->forward,
							vecAdd(
								vecScale(pix_x, x - cam->width  * 0.5f + jitter_x + 0.5f),
								vecScale(pix_y, y - cam->height * 0.5f + jitter_y + 0.5f)
							)
						);
	new_ray.direction = vecNormalize(pix_v);
	
	if (cam->aperture > 0.0f) {
		const float ft = cam->focus_distance / vecDot(new_ray.direction, cam->forward);
		const struct vector focus_point = alongRay(&new_ray, ft);
		const struct coord lens_point = coordScale(cam->aperture, randomCoordOnUnitDisc(sampler));
		new_ray.start = vecAdd(new_ray.start, vecAdd(vecScale(cam->right, lens_point.x), vecScale(cam->up, lens_point.y)));
		new_ray.direction = vecNormalize(vecSub(focus_point, new_ray.start));
	}
	//To world space
	transformRay(&new_ray, cam->composite.A);
	return new_ray;
}
