//
//  pathtrace.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "pathtrace.h"

#include <float.h>
#include "../datatypes/scene.h"
#include "../datatypes/camera.h"
#include "../accelerators/bvh.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/sphere.h"
#include "../datatypes/poly.h"
#include "../datatypes/mesh.h"
#include "samplers/sampler.h"
#include "sky.h"
#include "../datatypes/transforms.h"
#include "../renderer/instance.h"
#include "../nodes/shaders/background.h"

static inline void recompute_uv(struct hitRecord *isect, float offset) {
	struct vector ud = vec_normalize(isect->incident_dir);
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x) / 4.0f) + offset;
	float theta = acosf((-ud.y / r));
	
	float u = (phi / (PI / 2.0f));
	float v = theta / PI;
	
	u = wrap_min_max(u, 0.0f, 1.0f);
	v = wrap_min_max(v, 0.0f, 1.0f);
	
	isect->uv = (struct coord){ u, v };
}

static inline struct hitRecord getClosestIsect(struct lightRay *incidentRay, const struct world *scene, sampler *sampler) {
	//TODO: Consider passing in last instance idx + polygon to detect self-intersections?
	struct hitRecord isect = { .incident_dir = incidentRay->direction, .instIndex = -1, .distance = FLT_MAX, .polygon = NULL };
	if (traverse_top_level_bvh(scene->instances, scene->topLevel, incidentRay, &isect, sampler)) return isect;
	// Didn't hit anything. Recompute the UV for the background
	recompute_uv(&isect, scene->backgroundOffset);
	return isect;
}

struct color path_trace(const struct lightRay *incidentRay, const struct world *scene, int max_bounces, sampler *sampler) {
	struct color path_weight = g_white_color;
	struct color path_radiance = g_black_color; // Final path contribution "color"
	struct lightRay currentRay = *incidentRay;
	
	for (int bounce = 0; bounce <= max_bounces; ++bounce) {
		const struct hitRecord isect = getClosestIsect(&currentRay, scene, sampler);
		if (isect.instIndex < 0) {
			path_radiance = colorAdd(path_radiance, colorMul(path_weight, scene->background->sample(scene->background, sampler, &isect).weight));
			break;
		}
		
		//TODO: emission contribution needs to be adjusted down by probability of randomly hitting it
		path_radiance = colorAdd(path_radiance, colorMul(path_weight, *isect.emission));

		if (bounce == max_bounces) break;

		const struct bsdfSample sample = isect.bsdf->sample(isect.bsdf, sampler, &isect);
		currentRay = (struct lightRay){ .start = isect.hitPoint, .direction = sample.out };
		const struct color attenuation = sample.weight;
		
		// Russian Roulette - Abort a path early if it won't contribute much to the final image
		float rr_continue_probability = 1.0f;
		if (bounce >= 4) {
			rr_continue_probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
			if (getDimension(sampler) > rr_continue_probability)
				break;
		}
		
		path_weight = colorCoef(1.0f / rr_continue_probability, colorMul(attenuation, path_weight));
	}
	return path_radiance;
}
