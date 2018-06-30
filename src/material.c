//
//  material.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "material.h"

#include "pathtrace.h"

//FIXME: Temporary, eventually support full OBJ spec
struct material newMaterial(struct color diffuse, double reflectivity) {
	struct material newMaterial;
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

/*emission = 0,
 lambertian,
 glass,
 metal,
 translucent,
 transparent*/

void assignBSDF(struct material *mat) {
	switch (mat->type) {
		case lambertian:
			mat->bsdf = lambertianBSDF;
			break;
		case metal:
			mat->bsdf = metallicBSDF;
			break;
		default:
			mat->bsdf = lambertianBSDF;
			break;
	}
}

struct color colorForUV(struct material mtl, struct coord uv) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	//We need to combine the given uv, material texture coordinates, and magic to resolve this color.
	
	int x = (int)uv.x;
	int y = (int)uv.y;
	
	output.red = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 0];
	output.green = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 1];
	output.blue = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 2];
	
	return output;
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
		vec = vectorMultiply(vectorWithPos(drand48(), drand48(), drand48()), 2.0);
		struct vector temp = vectorWithPos(1.0, 1.0, 1.0);
		vec = subtractVectors(&vec, &temp);
	} while (squaredVectorLength(&vec) >= 1.0);
	return vec;
}

bool lambertianBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector temp = addVectors(&isect->hitPoint, &isect->surfaceNormal);
	struct vector rand = randomInUnitSphere();
	struct vector target = addVectors(&temp, &rand);
	
	struct vector target2 = subtractVectors(&isect->hitPoint, &target);
	
	*scattered = ((struct lightRay){isect->hitPoint, target2, rayTypeReflected, isect->end, 0});
	*attenuation = isect->end.diffuse;
	return true;
}

bool metallicBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector normalizedDir = normalizeVector(&isect->ray.direction);
	struct vector reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = isect->end.diffuse;
	return (scalarProduct(&scattered->direction, &isect->surfaceNormal) > 0);
}
