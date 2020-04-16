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


struct material newMaterialFull(struct color ambient,
								struct color diffuse,
								struct color specular,
								float reflectivity,
								float refractivity,
								float IOR,
								float transparency,
								float sharpness,
								float glossiness) {
	struct material mat;
	
	mat.ambient = ambient;
	mat.diffuse = diffuse;
	mat.specular = specular;
	mat.reflectivity = reflectivity;
	mat.refractivity = refractivity;
	mat.IOR = IOR;
	mat.transparency = transparency;
	mat.sharpness = sharpness;
	mat.glossiness = glossiness;
	
	return mat;
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
struct color colorForUV(struct hitRecord *isect) {
	struct color output;
	struct material mtl = isect->end;
	struct poly p = polygonArray[isect->polyIndex];
	
	//Texture width and height for this material
	float width = mtl.texture->width;
	float heigh = mtl.texture->height;

	//barycentric coordinates for this polygon
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0f - u - v;
	
	//Weighted texture coordinates
	struct coord ucomponent = coordScale(u, textureArray[p.textureIndex[2]]);
	struct coord vcomponent = coordScale(v, textureArray[p.textureIndex[1]]);
	struct coord wcomponent = coordScale(w, textureArray[p.textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	float x = (textureXY.x*(width));
	float y = (textureXY.y*(heigh));
	
	//Get the color value at these XY coordinates
	output = textureGetPixelFiltered(mtl.texture, x, y);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Maybe ask lodepng if we actually need to do this transform
	output = fromSRGB(output);
	
	return output;
}

struct color gradient(struct hitRecord *isect) {
	//barycentric coordinates for this polygon
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0 - u - v;
	
	return colorWithValues(u, v, w, 1.0f);
}

//FIXME: Make this configurable
//This is a checkerboard pattern mapped to the surface coordinate space
//Caveat: This only works for meshes that have texture coordinates (i.e. were UV-unwrapped).
struct color mappedCheckerBoard(struct hitRecord *isect, float coef) {
	ASSERT(isect->end.hasTexture);
	struct poly p = polygonArray[isect->polyIndex];
	
	//barycentric coordinates for this polygon
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0f - u - v;
	
	//Weighted coordinates
	struct coord ucomponent = coordScale(u, textureArray[p.textureIndex[2]]);
	struct coord vcomponent = coordScale(v, textureArray[p.textureIndex[1]]);
	struct coord wcomponent = coordScale(w,	textureArray[p.textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord surfaceXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
	
	float sines = sin(coef*surfaceXY.x) * sin(coef*surfaceXY.y);
	
	if (sines < 0.0f) {
		return (struct color){0.1f, 0.1f, 0.1f, 0.0};
	} else {
		return (struct color){0.4f, 0.4f, 0.4f, 0.0};
	}
}

//FIXME: Make this configurable
//This is a spatial checkerboard, mapped to the world coordinate space (always axis aligned)
struct color unmappedCheckerBoard(struct hitRecord *isect, float coef) {
	float sines = sin(coef*isect->hitPoint.x) * sin(coef*isect->hitPoint.y) * sin(coef*isect->hitPoint.z);
	if (sines < 0.0f) {
		return (struct color){0.1f, 0.1f, 0.1f, 0.0f};
	} else {
		return (struct color){0.4f, 0.4f, 0.4f, 0.0f};
	}
}

struct color checkerBoard(struct hitRecord *isect, float coef) {
	return isect->end.hasTexture ? mappedCheckerBoard(isect, coef) : unmappedCheckerBoard(isect, coef);
}

/**
 Compute reflection vector from a given vector and surface normal
 
 @param vec Incident ray to reflect
 @param normal Surface normal at point of reflection
 @return Reflected vector
 */
struct vector reflectVec(const struct vector *incident, const struct vector *normal) {
	float reflect = 2.0f * vecDot(*incident, *normal);
	return vecSub(*incident, vecScale(*normal, reflect));
}

struct vector randomInUnitSphere(pcg32_random_t *rng) {
	struct vector vec;
	do {
		vec = vecScale(vecWithPos(rndFloat(rng), rndFloat(rng), rndFloat(rng)), 2.0f);
		vec = vecSub(vec, (vector){1.0f, 1.0f, 1.0f});
	} while (vecLengthSquared(vec) >= 1.0f);
	return vec;
}

struct vector randomOnUnitSphere(pcg32_random_t *rng) {
	struct vector vec;
	do {
		vec = vecScale(vecWithPos(rndFloat(rng), rndFloat(rng), rndFloat(rng)), 2.0f);
		vec = vecSub(vec, (vector){1.0f, 1.0f, 1.0f});
	} while (vecLengthSquared(vec) >= 1.0f);
	return vecNormalize(vec);
}

bool emissiveBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	(void)isect;
	(void)attenuation;
	(void)scattered;
	(void)rng;
	return false;
}

bool weightedBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	(void)isect;
	(void)attenuation;
	(void)scattered;
	(void)rng;
	/*
	 This will be the internal shader weighting solver that runs a random distribution and chooses from the available
	 discrete shaders.
	 */
	
	return false;
}

//TODO: Make this a function ptr in the material?
struct color diffuseColor(struct hitRecord *isect) {
	return isect->end.hasTexture ? colorForUV(isect) : isect->end.diffuse;
}

bool lambertianBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector temp = vecAdd(isect->hitPoint, isect->surfaceNormal);
	struct vector rand = randomInUnitSphere(rng);
	struct vector scatterDir = vecSub(vecAdd(temp, rand), isect->hitPoint); //Randomized scatter direction
	*scattered = ((struct lightRay){isect->hitPoint, scatterDir, rayTypeScattered});
	*attenuation = diffuseColor(isect);
	return true;
}

bool metallicBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector normalizedDir = vecNormalize(isect->incident.direction);
	struct vector reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
	//Roughness
	if (isect->end.roughness > 0.0f) {
		struct vector fuzz = vecScale(randomInUnitSphere(rng), isect->end.roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = diffuseColor(isect);
	return (vecDot(scattered->direction, isect->surfaceNormal) > 0.0f);
}

bool refract(struct vector in, struct vector normal, float niOverNt, struct vector *refracted) {
	struct vector uv = vecNormalize(in);
	float dt = vecDot(uv, normal);
	float discriminant = 1.0f - niOverNt * niOverNt * (1.0f - dt * dt);
	if (discriminant > 0.0f) {
		struct vector A = vecScale(normal, dt);
		struct vector B = vecSub(uv, A);
		struct vector C = vecScale(B, niOverNt);
		struct vector D = vecScale(normal, sqrt(discriminant));
		*refracted = vecSub(C, D);
		return true;
	} else {
		return false;
	}
}

float shlick(float cosine, float IOR) {
	float r0 = (1.0f - IOR) / (1.0f + IOR);
	r0 = r0*r0;
	return r0 + (1.0f - r0) * powf((1.0f - cosine), 5.0f);
}

bool shinyBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	*attenuation = whiteColor;
	//Roughness
	if (isect->end.roughness > 0.0f) {
		struct vector fuzz = vecScale(randomInUnitSphere(rng), isect->end.roughness);
		reflected = vecAdd(reflected, fuzz);
	}
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	return true;
}

// Glossy plastic
bool plasticBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = diffuseColor(isect);
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	//TODO: Maybe don't hard code it like this.
	isect->end.IOR = 1.45f; // Car paint
	if (vecDot(isect->incident.direction, isect->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(isect->surfaceNormal);
		niOverNt = isect->end.IOR;
		cosine = isect->end.IOR * vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction);
	} else {
		outwardNormal = isect->surfaceNormal;
		niOverNt = 1.0f / isect->end.IOR;
		cosine = -(vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction));
	}
	
	if (refract(isect->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = shlick(cosine, isect->end.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0f;
	}
	
	if (rndFloat(rng) < reflectionProbability) {
		return shinyBSDF(isect, attenuation, scattered, rng);
	} else {
		return lambertianBSDF(isect, attenuation, scattered, rng);
	}
	return true;
}

// Only works on spheres for now. Reflections work but refractions don't
bool dielectricBSDF(struct hitRecord *isect, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->incident.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = diffuseColor(isect);
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(isect->incident.direction, isect->surfaceNormal) > 0.0f) {
		outwardNormal = vecNegate(isect->surfaceNormal);
		niOverNt = isect->end.IOR;
		cosine = isect->end.IOR * vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction);
	} else {
		outwardNormal = isect->surfaceNormal;
		niOverNt = 1.0f / isect->end.IOR;
		cosine = -(vecDot(isect->incident.direction, isect->surfaceNormal) / vecLength(isect->incident.direction));
	}
	
	if (refract(isect->incident.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = shlick(cosine, isect->end.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0f;
	}
	
	//Roughness
	if (isect->end.roughness > 0.0f) {
		struct vector fuzz = vecScale(randomInUnitSphere(rng), isect->end.roughness);
		reflected = vecAdd(reflected, fuzz);
		refracted = vecAdd(refracted, fuzz);
	}
	
	if (rndFloat(rng) < reflectionProbability) {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	} else {
		*scattered = newRay(isect->hitPoint, refracted, rayTypeRefracted);
	}
	return true;
}

void destroyMaterial(struct material *mat) {
	if (mat->textureFilePath) {
		free(mat->textureFilePath);
	}
	if (mat->normalMapPath) {
		free(mat->normalMapPath);
	}
	if (mat->name) {
		free(mat->name);
	}
	if (mat->hasTexture) {
		destroyTexture(mat->texture);
	}
}
