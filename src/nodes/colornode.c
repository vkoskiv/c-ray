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

const struct colorNode *unknownTextureNode(const struct node_storage *s) {
	return newConstantTexture(s, blackColor);
}

const struct colorNode *parseTextureNode(const char *asset_path, struct file_cache *cache, struct node_storage *s, const cJSON *node) {
	if (!node) return NULL;

	if (cJSON_IsArray(node)) {
		return newConstantTexture(s, parseColor(node));
	}

	// Handle options first
	uint8_t options = 0;
	options |= SRGB_TRANSFORM; // Enabled by default.

	if (cJSON_IsString(node)) {
		// No options provided, go with defaults.
		char *fullPath = stringConcat(asset_path, node->valuestring);
		windowsFixPath(fullPath);
		const struct colorNode *color_node = newImageTexture(s, load_texture(fullPath, &s->node_pool, cache), options);
		free(fullPath);
		return color_node;
	}

	// Should be an object, then.
	if (!cJSON_IsObject(node)) {
		logr(warning, "Invalid texture node given: \"%s\"\n", cJSON_PrintUnformatted(node));
		return unknownTextureNode(s);
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
		char *fullPath = stringConcat(asset_path, path->valuestring);
		windowsFixPath(fullPath);
		const struct colorNode *color_node = newImageTexture(s, load_texture(fullPath, &s->node_pool, cache), options);
		free(fullPath);
		return color_node;
	}

	//FIXME: No good way to know if it's a color, so just check if it's got "r" in there.
	if (cJSON_HasObjectItem(node, "r")) {
		// This is actually still a color object.
		return newConstantTexture(s, parseColor(node));
	}
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (cJSON_IsString(type)) {
		// Oo, what's this?
		if (stringEquals(type->valuestring, "checkerboard")) {
			const struct colorNode *a = parseTextureNode(asset_path, cache, s, cJSON_GetObjectItem(node, "color1"));
			const struct colorNode *b = parseTextureNode(asset_path, cache, s, cJSON_GetObjectItem(node, "color2"));
			const struct valueNode *scale = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "scale"));
			return newCheckerBoardTexture(s, a, b, scale);
		}
		if (stringEquals(type->valuestring, "blackbody")) {
			const cJSON *degrees = cJSON_GetObjectItem(node, "degrees");
			ASSERT(cJSON_IsNumber(degrees));
			return newBlackbody(s, newConstantValue(s, degrees->valuedouble));
		}
		if (stringEquals(type->valuestring, "split")) {
			return newSplitValue(s, parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "constant")));
		}
		if (stringEquals(type->valuestring, "rgb")) {
			const struct valueNode *red   = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "r"));
			const struct valueNode *green = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "g"));
			const struct valueNode *blue  = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "b"));
			return newCombineRGB(s, red, green, blue);
		}
		if (stringEquals(type->valuestring, "hsl")) {
			const struct valueNode *H = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "h"));
			const struct valueNode *S = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "s"));
			const struct valueNode *L = parseValueNode(asset_path, cache, s, cJSON_GetObjectItem(node, "l"));
			return newCombineHSL(s, H, S, L);
		}
		if (stringEquals(type->valuestring, "to_color")) {
			return newVecToColor(s, parseVectorNode(s, cJSON_GetObjectItem(node, "vector")));
		}
	}

	logr(warning, "Failed to parse textureNode. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return unknownTextureNode(s);
}
