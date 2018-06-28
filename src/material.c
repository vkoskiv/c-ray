//
//  material.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "material.h"

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

/*bool lambertianBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered);
bool metallicBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered);

void assignBSDF(struct material *mat) {
	switch (mat->type) {
		case lambertian:
			mat->bsdf = lambertianBSDF;
			break;
		case metal:
			mat->bsdf = metallicBSDF;
			
		default:
			mat->bsdf = lambertianBSDF;
			break;
	}
}*/
