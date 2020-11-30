//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "material.h"

#include "../renderer/pathtrace.h"
#include "vertexbuffer.h"
#include "image/texture.h"
#include "poly.h"
#include "../utils/assert.h"
#include "../utils/logging.h"

#include "../renderer/bsdf/bsdf.h"

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

void assignBSDF(struct material *mat) {
	struct textureNode *roughness = mat->specularMap ? newImageTexture(mat->specularMap, Specular, 0) : newConstantTexture(colorWithValues(mat->roughness, 0, 0, 0));
	struct textureNode *color = mat->texture ? newImageTexture(mat->texture, Diffuse, 0) : newConstantTexture(mat->diffuse);
	switch (mat->type) {
		case lambertian:
			mat->bsdf = (struct bsdf*)newDiffuse(color);
			break;
		case glass:
			mat->bsdf = (struct bsdf*)newGlass(color, roughness);
			break;
		case metal:
			mat->bsdf = (struct bsdf*)newMetal(color, roughness);
			break;
		default:
			logr(warning, "Unknown bsdf type specified for \"%s\"\n", mat->name);
			mat->bsdf = (struct bsdf*)newDiffuse(newConstantTexture(warningMaterial().diffuse));
			break;
	}
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
	
	//Texture width and height for this material
	const float width = tex->width;
	const float heigh = tex->height;

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
	
	const float x = (textureXY.x*(width));
	const float y = (textureXY.y*(heigh));
	
	//Get the color value at these XY coordinates
	output = textureGetPixelFiltered(tex, x, y);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	if (type == Diffuse) output = fromSRGB(output);
	
	return output;
}

/*bool lambertianBSDF(const struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	const struct vector scatterDir = vecNormalize(vecAdd(isect->surfaceNormal, randomOnUnitSphere(sampler)));
	*scattered = ((struct lightRay){isect->hitPoint, scatterDir, rayTypeScattered});
	*attenuation = diffuseColor(isect);
	return true;
}*/

/*bool shinyBSDF(const struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	*attenuation = whiteColor;
	//Roughness
	float roughness = roughnessValue(isect);
	if (roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	return true;
}*/

// Glossy plastic
/*bool plasticBSDF(const struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = diffuseColor(isect);
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(isect->incident.direction, isect->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(isect->surfaceNormal);
		niOverNt = isect->material.IOR;
		cosine = isect->material.IOR * vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction);
	} else {
		outwardNormal = isect->surfaceNormal;
		niOverNt = 1.0f / isect->material.IOR;
		cosine = -(vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction));
	}
	
	if (refract(isect->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = schlick(cosine, isect->material.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0f;
	}
	
	if (getDimension(sampler) < reflectionProbability) {
		return shinyBSDF(isect, attenuation, scattered, sampler);
	} else {
		return lambertianBSDF(isect, attenuation, scattered, sampler);
	}
}*/

void destroyMaterial(struct material *mat) {
	if (mat) {
		free(mat->name);
		if (mat->texture) destroyTexture(mat->texture);
		if (mat->normalMap) destroyTexture(mat->normalMap);
		if (mat->specularMap) destroyTexture(mat->specularMap);
	}
	//FIXME: Free mat here and fix destroyMesh() to work with that
}
