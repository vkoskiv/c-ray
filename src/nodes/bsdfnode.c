//
//  bsdfnode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 29/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/vector.h"
#include "../datatypes/color.h"
#include "../datatypes/material.h"
#include "../renderer/renderer.h"
#include "../utils/string.h"
#include "bsdfnode.h"

const struct bsdfNode *warningBsdf(const struct world *world) {
	return newMix(world,
				  newDiffuse(world, newConstantTexture(world, warningMaterial().diffuse)),
				  newDiffuse(world, newConstantTexture(world, (struct color){0.2f, 0.2f, 0.2f, 1.0f})),
				  newGrayscaleConverter(world, newCheckerBoardTexture(world, NULL, NULL, newConstantValue(world, 500.0f))));
}

const struct bsdfNode *parseBsdfNode(struct renderer *r, const cJSON *node) {
	if (!node) return NULL;
	struct world *w = r->scene;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for bsdfNode.\n");
		return warningBsdf(w);
	}

	const struct colorNode *color = parseTextureNode(r, cJSON_GetObjectItem(node, "color"));
	const struct valueNode *roughness = parseValueNode(r, cJSON_GetObjectItem(node, "roughness"));
	const struct valueNode *strength = parseValueNode(r, cJSON_GetObjectItem(node, "strength"));
	const struct valueNode *IOR = parseValueNode(r, cJSON_GetObjectItem(node, "IOR"));
	const struct valueNode *factor = parseValueNode(r, cJSON_GetObjectItem(node, "factor"));
	const struct bsdfNode *A = parseBsdfNode(r, cJSON_GetObjectItem(node, "A"));
	const struct bsdfNode *B = parseBsdfNode(r, cJSON_GetObjectItem(node, "B"));

	if (stringEquals(type->valuestring, "diffuse")) {
		return newDiffuse(w, color);
	} else if (stringEquals(type->valuestring, "metal")) {
		return newMetal(w, color, roughness);
	} else if (stringEquals(type->valuestring, "glass")) {
		return newGlass(w, color, roughness, IOR);
	} else if (stringEquals(type->valuestring, "plastic")) {
		return newPlastic(w, color, roughness, IOR);
	} else if (stringEquals(type->valuestring, "mix")) {
		return newMix(w, A, B, factor);
	} else if (stringEquals(type->valuestring, "add")) {
		return newAdd(w, A, B);
	} else if (stringEquals(type->valuestring, "transparent")) {
		return newTransparent(w, color);
	} else if (stringEquals(type->valuestring, "emissive")) {
		return newEmission(w, color, strength);
	} else if (stringEquals(type->valuestring, "translucent")) {
		return newTranslucent(w, color);
	}

	logr(warning, "Failed to parse node. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return warningBsdf(w);
}