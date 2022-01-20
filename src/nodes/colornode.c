//
//  texturenode.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../datatypes/color.h"
#include "../renderer/samplers/sampler.h"
#include "../renderer/renderer.h"
#include "../datatypes/vector.h"
#include "../datatypes/material.h"
#include "../datatypes/poly.h"
#include "../utils/string.h"
#include "../datatypes/scene.h"
#include "../utils/loaders/textureloader.h"
#include "../utils/loaders/sceneloader.h"
#include "bsdfnode.h"

#include "colornode.h"

const struct colorNode *unknownTextureNode(const struct world *w) {
	return newConstantTexture(w, blackColor);
}

const struct colorNode *parseTextureNode(struct renderer *r, const cJSON *node) {
	if (!node) return NULL;

	struct world *w = r->scene;

	if (cJSON_IsArray(node)) {
		return newConstantTexture(w, parseColor(node));
	}

	// Handle options first
	uint8_t options = 0;
	options |= SRGB_TRANSFORM; // Enabled by default.

	if (cJSON_IsString(node)) {
		// No options provided, go with defaults.
		char *fullPath = stringConcat(r->prefs.assetPath, node->valuestring);
		windowsFixPath(fullPath);
		const struct colorNode *node = newImageTexture(w, load_texture(fullPath, &w->nodePool), options);
		free(fullPath);
		return node;
	}

	// Should be an object, then.
	if (!cJSON_IsObject(node)) {
		logr(warning, "Invalid texture node given: \"%s\"\n", cJSON_PrintUnformatted(node));
		return unknownTextureNode(w);
	}

	// Do we want to do an srgb transform?
	const cJSON *srgbTransform = cJSON_GetObjectItem(node, "transform");
	if (srgbTransform) {
		if (!cJSON_IsTrue(srgbTransform)) {
			options &= ~SRGB_TRANSFORM;
		}
	}

	// Do we want bilinear interpolation enabled?
	const cJSON *lerp = cJSON_GetObjectItem(node, "lerp");
	if (!cJSON_IsTrue(lerp)) {
		options |= NO_BILINEAR;
	}

	const cJSON *path = cJSON_GetObjectItem(node, "path");
	if (cJSON_IsString(path)) {
		char *fullPath = stringConcat(r->prefs.assetPath, path->valuestring);
		windowsFixPath(fullPath);
		const struct colorNode *node = newImageTexture(w, load_texture(fullPath, &w->nodePool), options);
		free(fullPath);
		return node;
	}

	//FIXME: No good way to know if it's a color, so just check if it's got "r" in there.
	if (cJSON_HasObjectItem(node, "r")) {
		// This is actually still a color object.
		return newConstantTexture(w, parseColor(node));
	}
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (cJSON_IsString(type)) {
		// Oo, what's this?
		if (stringEquals(type->valuestring, "checkerboard")) {
			const struct colorNode *a = parseTextureNode(r, cJSON_GetObjectItem(node, "color1"));
			const struct colorNode *b = parseTextureNode(r, cJSON_GetObjectItem(node, "color2"));
			const struct valueNode *scale = parseValueNode(r, cJSON_GetObjectItem(node, "scale"));
			return newCheckerBoardTexture(w, a, b, scale);
		}
		if (stringEquals(type->valuestring, "blackbody")) {
			const cJSON *degrees = cJSON_GetObjectItem(node, "degrees");
			ASSERT(cJSON_IsNumber(degrees));
			return newBlackbody(w, newConstantValue(w, degrees->valuedouble));
		}
		if (stringEquals(type->valuestring, "split")) {
			return newSplitValue(w, parseValueNode(r, cJSON_GetObjectItem(node, "constant")));
		}
		if (stringEquals(type->valuestring, "combine")) {
			const struct valueNode *red = parseValueNode(r, cJSON_GetObjectItem(node, "r"));
			const struct valueNode *green = parseValueNode(r, cJSON_GetObjectItem(node, "g"));
			const struct valueNode *blue = parseValueNode(r, cJSON_GetObjectItem(node, "b"));
			return newCombineRGB(w, red, green, blue);
		}
		if (stringEquals(type->valuestring, "to_color")) {
			return newVecToColor(w, parseVectorNode(w, cJSON_GetObjectItem(node, "vector")));
		}
	}

	logr(warning, "Failed to parse textureNode. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return unknownTextureNode(w);
}
