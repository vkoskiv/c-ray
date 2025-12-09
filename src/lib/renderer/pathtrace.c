//
//  pathtrace.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "pathtrace.h"

#include <float.h>
#include <datatypes/scene.h>
#include <datatypes/poly.h>
#include <renderer/instance.h>
#include <accelerators/bvh.h>
#include <common/transforms.h>
#include "samplers/sampler.h"
#include "sky.h"

static inline struct hitRecord getClosestIsect(struct lightRay *incidentRay, const struct world *scene, sampler *sampler) {
	//TODO: Consider passing in last instance idx + polygon to detect self-intersections?
	struct hitRecord isect = { .incident = incidentRay, .instIndex = -1, .distance = FLT_MAX, .polygon = NULL };
	traverse_top_level_bvh(scene->instances, scene->topLevel, incidentRay, &isect, sampler);
	return isect;
}

struct color path_trace(struct lightRay incident, const struct world *scene, int max_bounces, sampler *sampler) {
	struct color path_weight = g_white_color;
	struct color path_radiance = g_black_color; // Final path contribution "color"
	struct lightRay currentRay = incident;

	for (int bounce = 0; bounce <= max_bounces; ++bounce) {
		const struct hitRecord isect = getClosestIsect(&currentRay, scene, sampler);
		if (isect.instIndex < 0) {
			path_radiance = colorAdd(path_radiance, colorMul(path_weight, scene->background->sample(scene->background, sampler, &isect).weight));
			break;
		}
		
		const struct bsdfSample sample = isect.bsdf->sample(isect.bsdf, sampler, &isect);
		//TODO: emission contribution needs to be adjusted down by probability of randomly hitting it
		//FIXME: emits_light only gets set if the root node of a shader graph is emissive, so maybe fix that
		// if (true || scene->instances[isect.instIndex].emits_light) {
			path_radiance = colorAdd(path_radiance, colorMul(path_weight, sample.emitted));
		// }
		if (bounce == max_bounces) break;

		currentRay = sample.out;
		const struct color attenuation = sample.weight;
		
		// Russian Roulette - Abort a path early if it won't contribute much to the final image
		float rr_continue_probability = 1.0f;
		if (bounce >= 4) {
			rr_continue_probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
			if (sampler_dimension(sampler) > rr_continue_probability)
				break;
		}
		
		path_weight = colorCoef(1.0f / rr_continue_probability, colorMul(attenuation, path_weight));
	}
	return path_radiance;
}
