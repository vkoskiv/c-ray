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
	struct vector ud = vecNormalize(isect->incident_dir);
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x) / 4.0f) + offset;
	float theta = acosf((-ud.y / r));
	
	float u = (phi / (PI / 2.0f));
	float v = theta / PI;
	
	u = wrapMinMax(u, 0.0f, 1.0f);
	v = wrapMinMax(v, 0.0f, 1.0f);
	
	isect->uv = (struct coord){ u, v };
}

static inline struct hitRecord getClosestIsect(struct lightRay *incidentRay, const struct world *scene, sampler *sampler) {
	struct hitRecord isect = { .incident_dir = incidentRay->direction, .instIndex = -1, .distance = FLT_MAX, .polygon = NULL };
	if (traverseTopLevelBvh(scene->instances, scene->topLevel, incidentRay, &isect, sampler)) return isect;
	// Didn't hit anything. Recompute the UV for the background
	recompute_uv(&isect, scene->backgroundOffset);
	return isect;
}

struct color pathTrace(const struct lightRay *incidentRay, const struct world *scene, int maxDepth, sampler *sampler) {
	struct color weight = whiteColor; // Current path weight
	struct color finalColor = blackColor; // Final path contribution
	struct lightRay currentRay = *incidentRay;
	
	for (int depth = 0; depth < maxDepth; ++depth) {
		const struct hitRecord isect = getClosestIsect(&currentRay, scene, sampler);
		if (isect.instIndex < 0) {
			finalColor = colorAdd(finalColor, colorMul(weight, scene->background->sample(scene->background, sampler, &isect).color));
			break;
		}
		
		finalColor = colorAdd(finalColor, colorMul(weight, *isect.emission));
		
		const struct bsdfSample sample = isect.bsdf->sample(isect.bsdf, sampler, &isect);
		currentRay = (struct lightRay){ .start = isect.hitPoint, .direction = sample.out };
		const struct color attenuation = sample.color;
		
		// Russian Roulette - Abort a path early if it won't contribute much to the final image
		float rr_continue_probability = 1.0f;
		if (depth >= 4) {
			rr_continue_probability = max(attenuation.red, max(attenuation.green, attenuation.blue));
			if (getDimension(sampler) > rr_continue_probability)
				break;
		}
		
		weight = colorCoef(1.0f / rr_continue_probability, colorMul(attenuation, weight));
	}
	return finalColor;
}
