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

const struct bsdfNode *warningBsdf(const struct node_storage *s) {
	return newMix(s,
				  newDiffuse(s, newConstantTexture(s, warningMaterial().diffuse)),
				  newDiffuse(s, newConstantTexture(s, (struct color){0.2f, 0.2f, 0.2f, 1.0f})),
				  newGrayscaleConverter(s, newCheckerBoardTexture(s, NULL, NULL, newConstantValue(s, 500.0f))));
}

const struct bsdfNode *parseBsdfNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node) {
	if (!node) return NULL;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for bsdfNode.\n");
		return warningBsdf(s);
	}

	const struct colorNode *color = parseTextureNode(asset_path, cache, s, cJSON_GetObjectItem(node, "color"));
	const struct valueNode *roughness = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "roughness"));
	const struct valueNode *strength = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "strength"));
	const struct valueNode *IOR = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "IOR"));
	const struct valueNode *factor = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "factor"));
	const struct bsdfNode *A = parseBsdfNode(asset_path, cache, s, cJSON_GetObjectItem(node, "A"));
	const struct bsdfNode *B = parseBsdfNode(asset_path, cache, s, cJSON_GetObjectItem(node, "B"));

	if (stringEquals(type->valuestring, "diffuse")) {
		return newDiffuse(s, color);
	} else if (stringEquals(type->valuestring, "metal")) {
		return newMetal(s, color, roughness);
	} else if (stringEquals(type->valuestring, "glass")) {
		return newGlass(s, color, roughness, IOR);
	} else if (stringEquals(type->valuestring, "plastic")) {
		return newPlastic(s, color, roughness, IOR);
	} else if (stringEquals(type->valuestring, "mix")) {
		return newMix(s, A, B, factor);
	} else if (stringEquals(type->valuestring, "add")) {
		return newAdd(s, A, B);
	} else if (stringEquals(type->valuestring, "transparent")) {
		return newTransparent(s, color);
	} else if (stringEquals(type->valuestring, "emissive")) {
		return newEmission(s, color, strength);
	} else if (stringEquals(type->valuestring, "translucent")) {
		return newTranslucent(s, color);
	}

	logr(warning, "Failed to parse node. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return warningBsdf(s);
}