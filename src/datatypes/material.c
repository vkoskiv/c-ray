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

#include "../datatypes/color.h"

#include "../nodes/shaders/bsdf.h"

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

struct bsdf *warningBsdf(struct block **pool) {
	return newMix(pool,
				  newDiffuse(pool, newConstantTexture(pool, warningMaterial().diffuse)),
				  newDiffuse(pool, newConstantTexture(pool, (struct color){0.2f, 0.2f, 0.2f, 1.0f})),
				  newColorCheckerBoardTexture(pool, newConstantTexture(pool, blackColor), newConstantTexture(pool, whiteColor), 500.0f));
}

void assignBSDF(struct block **p, struct material *mat) {
	struct textureNode *roughness = mat->specularMap ? newImageTexture(pool, mat->specularMap, NO_BILINEAR) : newConstantTexture(pool, newGrayColor(mat->roughness));
	struct textureNode *color = mat->texture ? newImageTexture(pool, mat->texture, SRGB_TRANSFORM) : newConstantTexture(pool, mat->diffuse);
	switch (mat->type) {
		case lambertian:
			mat->bsdf = newDiffuse(pool, color);
			break;
		case glass:
			mat->bsdf = newGlass(pool, color, roughness);
			break;
		case metal:
			mat->bsdf = newMetal(pool, color, roughness);
			break;
		case plastic:
			mat->bsdf = newPlastic(pool, color);
			break;
		case emission:
			mat->bsdf = newDiffuse(pool, color);
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\", setting to an obnoxious preset.\n", mat->name);
			mat->bsdf = warningBsdf(pool);
			break;
	}
	ASSERT(mat->bsdf);
}

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
struct color colorForUV(const struct hitRecord *isect, enum textureType type) {
	struct color output;
	const struct texture *tex = NULL;
	switch (type) {
		case Normal:
			tex = isect->material.normalMap ? isect->material.normalMap : NULL;
			break;
		case Specular:
			tex = isect->material.specularMap ? isect->material.specularMap : NULL;
			break;
		default:
			tex = isect->material.texture ? isect->material.texture : NULL;
			break;
	}
	
	if (!tex) return warningMaterial().diffuse;
	
	const struct poly *p = isect->polygon;

	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	const struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	const struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	//Get the color value at these XY coordinates
	output = textureGetPixel(tex, textureXY.x, textureXY.y, true);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	if (type == Diffuse) output = fromSRGB(output);
	
	return output;
}

void destroyMaterial(struct material *mat) {
	if (mat) {
		free(mat->name);
		if (mat->texture) destroyTexture(mat->texture);
		if (mat->normalMap) destroyTexture(mat->normalMap);
		if (mat->specularMap) destroyTexture(mat->specularMap);
	}
	//FIXME: Free mat here and fix destroyMesh() to work with that
}
