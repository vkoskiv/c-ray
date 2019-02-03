//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "material.h"

#include "pathtrace.h"

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
			mat->bsdf = dialectric;
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
	double w = 1.0 - u - v; //1.0 - u - v
	
	//Weighted texture coordinates
	struct coord ucomponent = coordScale(u, &textureArray[p.textureIndex[1]]);
	struct coord vcomponent = coordScale(v, &textureArray[p.textureIndex[2]]);
	struct coord wcomponent = coordScale(w,	&textureArray[p.textureIndex[0]]);
	
	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord temp = addCoords(&ucomponent, &vcomponent);
	struct coord textureXY = addCoords(&temp, &wcomponent);
	
	//Final integer X Y texture pixel coords
	int x = (int)(textureXY.x*(width)) + 1;
	int y = (int)(textureXY.y*(heigh)) + 1;
	
	//Get the color value at these XY coordinates
	//These need to be normalized from 0-255 to 0-1 double (just divide by 255.0)
	output.red = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 0] / 255.0;
	output.green = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 1] / 255.0;
	output.blue = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 2] / 255.0;
	
	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Maybe ask lodepng if we actually need to do this transform
	output = fromSRGB(output);
	
	return output;
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
	double reflect = 2.0 * scalarProduct(incident, normal);
	struct vector temp = vectorScale(reflect, normal);
	return subtractVectors(incident, &temp);
}

struct vector randomInUnitSphere() {
	struct vector vec = (struct vector){0.0, 0.0, 0.0, false};
	do {
		vec = vectorMultiply(vectorWithPos(getRandomDouble(0, 1), getRandomDouble(0, 1), getRandomDouble(0, 1)), 2.0);
		struct vector temp = vectorWithPos(1.0, 1.0, 1.0);
		vec = subtractVectors(&vec, &temp);
	} while (squaredVectorLength(&vec) >= 1.0);
	return vec;
}

bool emissiveBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	return false;
}

struct color diffuseColor(struct intersection *isect) {
	if (isect->end.hasTexture) {
		return colorForUV(isect);
	} else {
		return isect->end.diffuse;
	}
}

bool lambertianBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector temp = addVectors(&isect->hitPoint, &isect->surfaceNormal);
	struct vector rand = randomInUnitSphere();
	struct vector target = addVectors(&temp, &rand);
	struct vector target2 = subtractVectors(&isect->hitPoint, &target);
	*scattered = ((struct lightRay){isect->hitPoint, target2, rayTypeReflected, isect->end, 0});
	*attenuation = diffuseColor(isect);
	return true;
}

bool metallicBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector normalizedDir = normalizeVector(&isect->ray.direction);
	struct vector reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = isect->end.diffuse;
	return (scalarProduct(&scattered->direction, &isect->surfaceNormal) > 0);
}

bool refract(struct vector in, struct vector normal, float niOverNt, struct vector *refracted) {
	struct vector uv = normalizeVector(&in);
	float dt = scalarProduct(&uv, &normal);
	float discriminant = 1.0 - niOverNt * niOverNt * (1 - dt * dt);
	if (discriminant > 0) {
		struct vector A = vectorMultiply(normal, dt);
		struct vector B = subtractVectors(&uv, &A);
		struct vector C = vectorMultiply(B, niOverNt);
		struct vector D = vectorMultiply(normal, sqrt(discriminant));
		*refracted = subtractVectors(&C, &D);
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

bool dialectric(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector outwardNormal;
	struct vector reflected = reflectVec(&isect->ray.direction, &isect->surfaceNormal);
	float niOverNt;
	*attenuation = isect->end.diffuse;
	struct vector refracted;
	float reflectionProbability;
	float cosine;
	
	if (scalarProduct(&isect->ray.direction, &isect->surfaceNormal) > 0) {
		outwardNormal = negateVector(isect->surfaceNormal);
		niOverNt = isect->end.IOR;
		cosine = isect->end.IOR * scalarProduct(&isect->ray.direction, &isect->surfaceNormal) / vectorLength(&isect->ray.direction);
	} else {
		outwardNormal = isect->surfaceNormal;
		niOverNt = 1.0 / isect->end.IOR;
		cosine = -(scalarProduct(&isect->ray.direction, &isect->surfaceNormal) / vectorLength(&isect->ray.direction));
	}
	
	if (refract(isect->ray.direction, outwardNormal, niOverNt, &refracted)) {
		reflectionProbability = shlick(cosine, isect->end.IOR);
	} else {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
		reflectionProbability = 1.0;
	}
	
	if (getRandomDouble(0, 1) < reflectionProbability) {
		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	} else {
		*scattered = newRay(isect->hitPoint, refracted, rayTypeRefracted);
	}
	return true;
}

void freeTexture(struct texture *tex) {
	if (tex->height) {
		free(tex->height);
	}
	if (tex->width) {
		free(tex->width);
	}
	if (tex->imgData) {
		free(tex->imgData);
	}
}

void freeMaterial(struct material *mat) {
	if (mat->textureFilePath) {
		free(mat->textureFilePath);
	}
	if (mat->name) {
		free(mat->name);
	}
}
