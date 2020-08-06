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

//FIXME: Temporary, eventually support full OBJ spec
struct material newMaterial(struct color diffuse, float reflectivity) {
	struct material newMaterial = {0};
	newMaterial.reflectivity = reflectivity;
	newMaterial.diffuse = diffuse;
	return newMaterial;
}

struct material emptyMaterial() {
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
	newMat.diffuse = (struct color){1.0f, 0.0f, 0.5f, 0.0f};
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
	//TODO: Add the BSDF weighting here
	switch (mat->type) {
		case lambertian:
			mat->bsdf = lambertianBSDF;
			break;
		case metal:
			mat->bsdf = metallicBSDF;
			break;
		case emission:
			mat->bsdf = emissiveBSDF;
			break;
		case glass:
			mat->bsdf = dielectricBSDF;
			break;
		case plastic:
			mat->bsdf = plasticBSDF;
			break;
		default:
			mat->bsdf = lambertianBSDF;
			break;
	}
}

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
static struct color colorForUV(struct hitRecord *isect) {
	struct color output;
	const struct material mtl = isect->material;
	const struct poly *p = isect->polygon;
	
	//Texture width and height for this material
	const float width = mtl.texture->width;
	const float heigh = mtl.texture->height;

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
	output = textureGetPixelFiltered(mtl.texture, x, y);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Maybe ask lodepng if we actually need to do this transform
	output = fromSRGB(output);
	
	return output;
}

static struct color gradient(struct hitRecord *isect) {
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	return colorWithValues(u, v, w, 1.0f);
}

//FIXME: Make this configurable
//This is a checkerboard pattern mapped to the surface coordinate space
//Caveat: This only works for meshes that have texture coordinates (i.e. were UV-unwrapped).
static struct color mappedCheckerBoard(struct hitRecord *isect, float coef) {
	ASSERT(isect->material.hasTexture);
	const struct poly *p = isect->polygon;
	
	//barycentric coordinates for this polygon
	const float u = isect->uv.x;
	const float v = isect->uv.y;
	const float w = 1.0f - u - v;
	
	//Weighted coordinates
	const struct coord ucomponent = coordScale(u, g_textureCoords[p->textureIndex[1]]);
	const struct coord vcomponent = coordScale(v, g_textureCoords[p->textureIndex[2]]);
	const struct coord wcomponent = coordScale(w, g_textureCoords[p->textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	const struct coord surfaceXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	const float sines = sinf(coef*surfaceXY.x) * sinf(coef*surfaceXY.y);
	
	if (sines < 0.0f) {
		return (struct color){0.1f, 0.1f, 0.1f, 0.0f};
	} else {
		return (struct color){0.4f, 0.4f, 0.4f, 0.0f};
	}
}

//FIXME: Make this configurable
//This is a spatial checkerboard, mapped to the world coordinate space (always axis aligned)
static struct color unmappedCheckerBoard(struct hitRecord *isect, float coef) {
	const float sines = sinf(coef*isect->hitPoint.x) * sinf(coef*isect->hitPoint.y) * sinf(coef*isect->hitPoint.z);
	if (sines < 0.0f) {
		return (struct color){0.1f, 0.1f, 0.1f, 0.0f};
	} else {
		return (struct color){0.4f, 0.4f, 0.4f, 0.0f};
	}
}

static struct color checkerBoard(struct hitRecord *isect, float coef) {
	return isect->material.hasTexture ? mappedCheckerBoard(isect, coef) : unmappedCheckerBoard(isect, coef);
}

static struct vector reflectVec(const struct vector *incident, const struct vector *normal) {
	const float reflect = 2.0f * vecDot(*incident, *normal);
	return vecSub(*incident, vecScale(*normal, reflect));
}

static struct vector randomOnUnitSphere(sampler *sampler) {
	const float sample_x = getDimension(sampler);
	const float sample_y = getDimension(sampler);
	const float a = sample_x * (2.0f * PI);
	const float s = 2.0f * sqrtf(max(0.0f, sample_y * (1.0f - sample_y)));
	return (struct vector){cosf(a) * s, sinf(a) * s, 1.0f - 2.0f * sample_y};
}

bool emissiveBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	(void)isect;
	(void)attenuation;
	(void)scattered;
	(void)sampler;
	return false;
}

bool weightedBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	(void)isect;
	(void)attenuation;
	(void)scattered;
	(void)sampler;
	/*
	 This will be the internal shader weighting solver that runs a random distribution and chooses from the available
	 discrete shaders.
	 */
	
	return false;
}

//TODO: Make this a function ptr in the material?
static struct color diffuseColor(struct hitRecord *isect) {
	return isect->material.hasTexture ? colorForUV(isect) : isect->material.diffuse;
}

bool lambertianBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	//struct vector temp = vecAdd(isect->hitPoint, isect->surfaceNormal);
	//struct vector rand = randomOnUnitSphere(sampler);
	const struct vector scatterDir = vecNormalize(vecAdd(isect->surfaceNormal, randomOnUnitSphere(sampler)));//vecSub(vecAdd(temp, rand), isect->hitPoint); //Randomized scatter direction
	*scattered = ((struct lightRay){isect->hitPoint, scatterDir, rayTypeScattered});
	*attenuation = diffuseColor(isect);
	return true;
}

bool metallicBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	const struct vector normalizedDir = vecNormalize(isect->incident.direction);
	struct vector reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
	//Roughness
	if (isect->material.roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), isect->material.roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = diffuseColor(isect);
	return (vecDot(scattered->direction, isect->surfaceNormal) > 0.0f);
}

static bool refract(struct vector in, struct vector normal, float niOverNt, struct vector *refracted) {
	const struct vector uv = vecNormalize(in);
	const float dt = vecDot(uv, normal);
	const float discriminant = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);
	if (discriminant > 0.0f) {
		const struct vector A = vecScale(normal, dt);
		const struct vector B = vecSub(uv, A);
		const struct vector C = vecScale(B, niOverNt);
		const struct vector D = vecScale(normal, sqrtf(discriminant));
		*refracted = vecSub(C, D);
		return true;
	} else {
		return false;
	}
}

static float schlick(float cosine, float IOR) {
	float r0 = (1.0f - IOR) / (1.0f + IOR);
	r0 = r0*r0;
	return r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
}

bool shinyBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	*attenuation = whiteColor;
	//Roughness
	if (isect->material.roughness > 0.0f) {
		const struct vector fuzz = vecScale(randomOnUnitSphere(sampler), isect->material.roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	return true;
}

// Glossy plastic
bool plasticBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = diffuseColor(isect);
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	//TODO: Maybe don't hard code it like this.
	isect->material.IOR = 1.45f; // Car paint
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
}

// Only works on spheres for now. Reflections work but refractions don't
bool dielectricBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, sampler *sampler) {
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
	
	//Roughness
	if (isect->material.roughness > 0.0f) {
		struct vector fuzz = vecScale(randomOnUnitSphere(sampler), isect->material.roughness);
		reflected = vecAdd(reflected, fuzz);
		refracted = vecAdd(refracted, fuzz);
	}
	
	if (getDimension(sampler) < reflectionProbability) {
		if (isect->material.roughness > 0.0f) {
			struct vector fuzz = vecScale(randomOnUnitSphere(sampler), isect->material.roughness);
			reflected = vecAdd(reflected, fuzz);
		}
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	} else {
		if (isect->material.roughness > 0.0f) {
			struct vector fuzz = vecScale(randomOnUnitSphere(sampler), isect->material.roughness);
			refracted = vecAdd(refracted, fuzz);
		}
		*scattered = newRay(isect->hitPoint, refracted, rayTypeRefracted);
	}
	return true;
}

void destroyMaterial(struct material *mat) {
	if (mat) {
		free(mat->textureFilePath);
		free(mat->normalMapPath);
		free(mat->name);
		if (mat->hasTexture) {
			destroyTexture(mat->texture);
		}
	}
}
