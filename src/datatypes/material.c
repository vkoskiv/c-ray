//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "../utils/mempool.h"
#include "material.h"

#include "../renderer/pathtrace.h"
#include "vertexbuffer.h"
#include "image/texture.h"
#include "poly.h"
#include "../utils/assert.h"
#include "../utils/logging.h"
#include "../datatypes/scene.h"

#include "../datatypes/color.h"

#include "../nodes/bsdfnode.h"

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
		if (strcmp(materials[i].name, name) == 0) {
			return &materials[i];
		}
	}
	return NULL;
}

void assignBSDF(struct world *w, struct material *mat) {
	struct colorNode *roughness = mat->specularMap ? newImageTexture(w, mat->specularMap, NO_BILINEAR) : newConstantTexture(w, newGrayColor(mat->roughness));
	struct colorNode *color = mat->texture ? newImageTexture(w, mat->texture, SRGB_TRANSFORM) : newConstantTexture(w, mat->diffuse);
	switch (mat->type) {
		case lambertian:
			mat->bsdf = newDiffuse(w, color);
			break;
		case glass:
			mat->bsdf = newGlass(w, color, roughness, mat->IOR);
			break;
		case metal:
			mat->bsdf = newMetal(w, color, roughness);
			break;
		case plastic:
			mat->bsdf = newPlastic(w, color);
			break;
		case emission:
			mat->bsdf = newDiffuse(w, color);
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\", setting to an obnoxious preset.\n", mat->name);
			mat->bsdf = warningBsdf(w);
			break;
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
