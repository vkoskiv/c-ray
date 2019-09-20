//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "material.h"

#include "../renderer/pathtrace.h"
#include "../datatypes/vertexbuffer.h"
#include "../datatypes/texture.h"

//FIXME: Temporary, eventually support full OBJ spec
struct material newMaterial(struct color diffuse, double reflectivity) {
	struct material newMaterial = {0};
	newMaterial.reflectivity = reflectivity;
	newMaterial.diffuse = diffuse;
	return newMaterial;
}


struct material newMaterialFull(struct color ambient,
								struct color diffuse,
								struct color specular,
								double reflectivity,
								double refractivity,
								double IOR, double
								transparency, double
								sharpness, double
								glossiness) {
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
	newMat.reflectivity = 1.0;
	newMat.type = lambertian;
	newMat.IOR = 1.0;
	return newMat;
}

//To showcase missing .MTL file, for example
struct material warningMaterial() {
	struct material newMat = emptyMaterial();
	newMat.type = lambertian;
	newMat.diffuse = (struct color){1.0, 0.0, 0.5, 0.0};
	return newMat;
}

//Find material with a given name and return a pointer to it
struct material *materialForName(struct material *materials, int count, char *name) {
	for (int i = 0; i < count; i++) {
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
			mat->bsdf = dialectricBSDF;
			break;
		default:
			mat->bsdf = lambertianBSDF;
			break;
	}
}

//Transform the intersection coordinates to the texture coordinate space
//And grab the color at that point. Texture mapping.
struct color colorForUV(struct intersection *isect) {
	struct color output = {0.0,0.0,0.0,0.0};
	struct material mtl = isect->end;
	struct poly p = polygonArray[isect->polyIndex];
	
	//Texture width and height for this material
	double width = *mtl.texture->width;
	double heigh = *mtl.texture->height;
	
	//barycentric coordinates for this polygon
	double u = isect->uv.x;
	double v = isect->uv.y;
	double w = 1.0 - u - v;
	
	//Weighted texture coordinates
	struct coord ucomponent = coordScale(u, &textureArray[p.textureIndex[1]]);
	struct coord vcomponent = coordScale(v, &textureArray[p.textureIndex[2]]);
	struct coord wcomponent = coordScale(w,	&textureArray[p.textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord temp = addCoords(&ucomponent, &vcomponent);
	struct coord textureXY = addCoords(&temp, &wcomponent);
	
	//Final integer X Y texture pixel coords
	int x = (int)(textureXY.x*(width));
	int y = (int)(textureXY.y*(heigh));
	
	//Somehow textureXY can range from -2.36 to 4.53. Not cool at all.
	//FIXME: Find out why this happens.
	//Clamp 'em for now
	x = x < 0 ? 0 : x;
	y = y < 0 ? 0 : y;
	x = x > width ? width : x;
	y = y > heigh ? heigh : y;
	
	//Get the color value at these XY coordinates
	output = textureGetPixel(mtl.texture, x, y);
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Maybe ask lodepng if we actually need to do this transform
	output = fromSRGB(output);
	
	return output;
}

struct color gradient(struct intersection *isect) {
	//barycentric coordinates for this polygon
	double u = isect->uv.x;
	double v = isect->uv.y;
	double w = 1.0 - u - v; //1.0 - u - v
	
	return colorWithValues(u, v, w, 1.0);
}

//FIXME: Make this configurable
//This is a checkerboard pattern mapped to the surface coordinate space
struct color mappedCheckerBoard(struct intersection *isect, float coef) {
	struct poly p = polygonArray[isect->polyIndex];
	
	//barycentric coordinates for this polygon
	double u = isect->uv.x;
	double v = isect->uv.y;
	double w = 1.0 - u - v; //1.0 - u - v
	
	//Weighted coordinates
	struct coord ucomponent = coordScale(u, &textureArray[p.textureIndex[1]]);
	struct coord vcomponent = coordScale(v, &textureArray[p.textureIndex[2]]);
	struct coord wcomponent = coordScale(w,	&textureArray[p.textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord temp = addCoords(&ucomponent, &vcomponent);
	struct coord surfaceXY = addCoords(&temp, &wcomponent);
	
	float sines = sin(coef*surfaceXY.x) * sin(coef*surfaceXY.y);
	
	if (sines < 0) {
		return (struct color){0.4, 0.4, 0.4, 0.0};
	} else {
		return (struct color){1.0, 1.0, 1.0, 0.0};
	}
}

//FIXME: Make this configurable
//This is a spatial checkerboard, mapped to the world coordinate space (always axis aligned)
struct color checkerBoard(struct intersection *isect, float coef) {
	float sines = sin(coef*isect->hitPoint.x) * sin(coef*isect->hitPoint.y) * sin(coef*isect->hitPoint.z);
	if (sines < 0) {
		return (struct color){0.4, 0.4, 0.4, 0.0};
	} else {
		return (struct color){1.0, 1.0, 1.0, 0.0};
	}
}

/**
 Compute reflection vector from a given vector and surface normal
 
 @param vec Incident ray to reflect
 @param normal Surface normal at point of reflection
 @return Reflected vector
 */
struct vector reflectVec(const struct vector *incident, const struct vector *normal) {
	double reflect = 2.0 * vecDot(incident, normal);
	struct vector temp = vecScale(reflect, normal);
	return vecSubtract(incident, &temp);
}

struct vector randomInUnitSphere(pcg32_random_t *rng) {
	struct vector vec = (struct vector){0.0, 0.0, 0.0};
	do {
		vec = vecMultiplyConst(vecWithPos(rndDouble(0, 1, rng), rndDouble(0, 1, rng), rndDouble(0, 1, rng)), 2.0);
		struct vector temp = vecWithPos(1.0, 1.0, 1.0);
		vec = vecSubtract(&vec, &temp);
	} while (vecLengthSquared(&vec) >= 1.0);
	return vec;
}

struct vector randomOnUnitSphere(pcg32_random_t *rng) {
	struct vector vec = (struct vector){0.0, 0.0, 0.0};
	do {
		vec = vecMultiplyConst(vecWithPos(rndDouble(0, 1, rng), rndDouble(0, 1, rng), rndDouble(0, 1, rng)), 2.0);
		struct vector temp = vecWithPos(1.0, 1.0, 1.0);
		vec = vecSubtract(&vec, &temp);
	} while (vecLengthSquared(&vec) >= 1.0);
	return vecNormalize(&vec);
}

bool emissiveBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	return false;
}

bool weightedBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	
	/*
	 This will be the internal shader weighting solver that runs a random distribution and chooses from the available
	 discrete shaders.
	 */
	
	return false;
}

//TODO: Make this a function ptr in the material?
struct color diffuseColor(struct intersection *isect) {
	if (isect->end.hasTexture) {
		return colorForUV(isect);
	} else {
		return isect->end.diffuse;
	}
}

bool lambertianBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector temp = vecAdd(&isect->hitPoint, &isect->surfaceNormal);
	struct vector rand = randomInUnitSphere(rng);
	struct vector target = vecAdd(&temp, &rand);
	struct vector target2 = vecSubtract(&isect->hitPoint, &target);
	*scattered = ((struct lightRay){isect->hitPoint, target2, rayTypeScattered, isect->end, 0});
	*attenuation = diffuseColor(isect);
	return true;
}

bool metallicBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector normalizedDir = vecNormalize(&isect->ray.direction);
	struct vector reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
	//Roughness
	struct vector fuzz = vecMultiplyConst(randomInUnitSphere(rng), isect->end.roughness);
	reflected = vecAdd(&reflected, &fuzz);
	
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = diffuseColor(isect);
	return (vecDot(&scattered->direction, &isect->surfaceNormal) > 0);
}

bool refract(struct vector in, struct vector normal, float niOverNt, struct vector *refracted) {
	struct vector uv = vecNormalize(&in);
	float dt = vecDot(&uv, &normal);
	float discriminant = 1.0 - niOverNt * niOverNt * (1 - dt * dt);
	if (discriminant > 0) {
		struct vector A = vecMultiplyConst(normal, dt);
		struct vector B = vecSubtract(&uv, &A);
		struct vector C = vecMultiplyConst(B, niOverNt);
		struct vector D = vecMultiplyConst(normal, sqrt(discriminant));
		*refracted = vecSubtract(&C, &D);
		return true;
	} else {
		return false;
	}
}

float shlick(float cosine, float IOR) {
	float r0 = (1 - IOR) / (1 + IOR);
	r0 = r0*r0;
	return r0 + (1 - r0) * pow((1 - cosine), 5);
}

// Only works on spheres for now. Reflections work but refractions don't
bool dialectricBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->ray.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = diffuseColor(isect);
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (vecDot(&isect->ray.direction, &isect->surfaceNormal) > 0) {
		outwardNormal = vecNegate(isect->surfaceNormal);
		niOverNt = isect->end.IOR;
		cosine = isect->end.IOR * vecDot(&isect->ray.direction, &isect->surfaceNormal) / vecLength(&isect->ray.direction);
	} else {
		outwardNormal = isect->surfaceNormal;
		niOverNt = 1.0 / isect->end.IOR;
		cosine = -(vecDot(&isect->ray.direction, &isect->surfaceNormal) / vecLength(&isect->ray.direction));
	}
	
	if (refract(isect->ray.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = shlick(cosine, isect->end.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0;
	}
	
	//Roughness
	struct vector fuzz = vecMultiplyConst(randomInUnitSphere(rng), isect->end.roughness);
	reflected = vecAdd(&reflected, &fuzz);
	refracted = vecAdd(&refracted, &fuzz);
	
	if (rndDouble(0, 1, rng) < reflectionProbability) {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	} else {
		*scattered = newRay(isect->hitPoint, refracted, rayTypeRefracted);
	}
	return true;
}

void freeMaterial(struct material *mat) {
	if (mat->textureFilePath) {
		free(mat->textureFilePath);
	}
	if (mat->name) {
		free(mat->name);
	}
}
