//
//  material.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//material
struct material {
	//char name[MATERIAL_NAME_SIZE];
	//char textureFilename[OBJ_FILENAME_LENGTH];
	struct color ambient;
	struct color diffuse;
	struct color specular;
	double reflectivity;
	double refractivity;
	double IOR;
	double transparency;
	double sharpness;
	double glossiness;
};

//temporary newMaterial func
struct material newMaterial(struct color diffuse, double reflectivity);

//Full obj spec material
struct material newMaterialFull(struct color ambient,
								struct color diffuse,
								struct color specular,
								double reflectivity,
								double refractivity,
								double IOR, double
								transparency, double
								sharpness, double
								glossiness);
