//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../utils/mempool.h"
#include "material.h"

#include "../renderer/pathtrace.h"
#include "image/texture.h"
#include "../datatypes/scene.h"

static struct material emptyMaterial() {
	return (struct material){0};
}

struct material defaultMaterial() {
	struct material newMat = emptyMaterial();
	newMat.diffuse = grayColor;
	newMat.reflectivity = 1.0f;
	newMat.type = lambertian;
	newMat.IOR = 1.0f;
	return newMat;
}

//To showcase missing .MTL file, for example
struct material warningMaterial() {
	struct material newMat = emptyMaterial();
	newMat.type = lambertian;
	newMat.diffuse = (struct color){1.0f, 0.0f, 0.5f, 1.0f};
	return newMat;
}

//FIXME: Temporary hack to patch alpha directly to old materials using the alpha node.
const struct bsdfNode *appendAlpha(const struct node_storage *s, const struct bsdfNode *base, const struct colorNode *color) {
	//FIXME: MSVC in release build mode crashes if we apply alpha on here, need to find out why. Just disable it for now though
#ifdef WINDOWS
	return base;
#else
	return newMix(s, newTransparent(s, newConstantTexture(s, whiteColor)), base, newAlpha(s, color));
#endif
}

void try_to_guess_bsdf(const struct node_storage *s, struct material *mat) {
	const struct valueNode *roughness = mat->specularMap ? newGrayscaleConverter(s, newImageTexture(s, mat->specularMap, NO_BILINEAR)) : newConstantValue(s, mat->roughness);
	const struct colorNode *color = mat->texture ? newImageTexture(s, mat->texture, SRGB_TRANSFORM) : newConstantTexture(s, mat->diffuse);
	logr(debug, "name: %s, illum: %i\n", mat->name, mat->illum);
	mat->bsdf = NULL;
	const struct bsdfNode *chosen_bsdf = NULL;
	
	const struct colorNode *spec = newConstantTexture(s, mat->specular);
	// First, attempt to deduce type based on mtl properties
	switch (mat->illum) {
		case 5:
			chosen_bsdf = appendAlpha(s, newMetal(s, color, roughness), color);
			break;
		case 7:
			chosen_bsdf = newGlass(s, spec, roughness, newConstantValue(s, mat->IOR));
			break;
		default:
			break;
	}
	
	if (chosen_bsdf) goto skip;
	
	// Otherwise, fall back to our preassigned selection
	switch (mat->type) {
		case lambertian:
			chosen_bsdf = newDiffuse(s, color);
			break;
		case glass:
			chosen_bsdf = newGlass(s, color, roughness, newConstantValue(s, mat->IOR));
			break;
		case metal:
			chosen_bsdf = newMetal(s, color, roughness);
			break;
		case plastic:
			chosen_bsdf = newPlastic(s, color, roughness, newConstantValue(s, mat->IOR));
			break;
		case emission:
			chosen_bsdf = newDiffuse(s, color);
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\", setting to an obnoxious preset.\n", mat->name);
			chosen_bsdf = warningBsdf(s);
			break;
	}

	skip:
	if (texture_uses_alpha(mat->texture)) {
		mat->bsdf = appendAlpha(s, chosen_bsdf, color);
	} else {
		mat->bsdf = chosen_bsdf;
	}

	ASSERT(mat->bsdf);
}

void destroyMaterial(struct material *mat) {
	if (mat) {
		free(mat->name);
		if (mat->texture) destroyTexture(mat->texture);
		if (mat->normalMap) destroyTexture(mat->normalMap);
		if (mat->specularMap) destroyTexture(mat->specularMap);
	}
}
