//
//  camera.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/03/2015.
//  Copyright © 2015-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <v.h>
#include <common/vector.h>
#include <common/quaternion.h>
#include <common/transforms.h>
#include "lightray.h"
#include "spline.h"

struct camera {
	float FOV;
	float focal_length;
	float focus_distance;
	float fstops;
	float aperture;
	float aspect_ratio;
	struct coord sensor_size;
	
	struct vector up;
	struct vector right;
	struct vector look_at;
	struct vector forward;
	
	struct transform composite;
	struct euler_angles orientation;
	struct vector position;
	
	struct spline *path;
	float time;
	
	// TODO: size_t
	int width;
	int height;

	bool is_blender;
};

typedef struct camera camera;
v_arr_def(camera)

struct sampler;

void cam_recompute_optics(struct camera *cam);
void cam_update_pose(struct camera *cam, const struct euler_angles *orientation, const struct vector *pos);
struct lightRay cam_get_ray(const struct camera *cam, int x, int y, struct sampler *sampler);
