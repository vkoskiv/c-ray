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
#include "../utils/string.h"
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

//Find material with a given name and return a pointer to it
struct material *materialForName(struct material *materials, int count, char *name) {
	for (int i = 0; i < count; ++i) {
		if (stringEquals(materials[i].name, name)) {
			return &materials[i];
		}
	}
	return NULL;
}

//FIXME: Temporary hack to patch alpha directly to old materials using the alpha node.
const struct bsdfNode *appendAlpha(struct world *w, const struct bsdfNode *base, const struct colorNode *color) {
	//FIXME: MSVC in release build mode crashes if we apply alpha on here, need to find out why. Just disable it for now though
#ifdef WINDOWS
	return base;
#else
	return newMix(w, newTransparent(w, newConstantTexture(w, whiteColor)), base, newAlpha(w, color));
#endif
}

void try_to_guess_bsdf(struct world *w, struct material *mat) {
	const struct valueNode *roughness = mat->specularMap ? newGrayscaleConverter(w, newImageTexture(w, mat->specularMap, NO_BILINEAR)) : newConstantValue(w, mat->roughness);
	const struct colorNode *color = mat->texture ? newImageTexture(w, mat->texture, SRGB_TRANSFORM) : newConstantTexture(w, mat->diffuse);
	logr(debug, "name: %s, illum: %i\n", mat->name, mat->illum);
	mat->bsdf = NULL;
	const struct bsdfNode *chosen_bsdf = NULL;
	
	const struct colorNode *spec = newConstantTexture(w, mat->specular);
	// First, attempt to deduce type based on mtl properties
	switch (mat->illum) {
		case 5:
			chosen_bsdf = appendAlpha(w, newMetal(w, color, roughness), color);
			break;
		case 7:
			chosen_bsdf = newGlass(w, spec, roughness, newConstantValue(w, mat->IOR));
			break;
		default:
			break;
	}
	
	if (chosen_bsdf) goto skip;
	
	// Otherwise, fall back to our preassigned selection
	switch (mat->type) {
		case lambertian:
			chosen_bsdf = newDiffuse(w, color);
			break;
		case glass:
			chosen_bsdf = newGlass(w, color, roughness, newConstantValue(w, mat->IOR));
			break;
		case metal:
			chosen_bsdf = newMetal(w, color, roughness);
			break;
		case plastic:
			chosen_bsdf = newPlastic(w, color, roughness, newConstantValue(w, mat->IOR));
			break;
		case emission:
			chosen_bsdf = newDiffuse(w, color);
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\", setting to an obnoxious preset.\n", mat->name);
			chosen_bsdf = warningBsdf(w);
			break;
	}

	skip:
	if (texture_uses_alpha(mat->texture)) {
		mat->bsdf = appendAlpha(w, chosen_bsdf, color);
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
