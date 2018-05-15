//
//  material.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/*
 From: https://blenderartists.org/forum/showthread.php?71202-Material-IOR-Value-reference
 'Air': 1.000
 'Bubble': 1.100
 'Liquid methane': 1.150
 'Ice(H2O)': 1.310
 'Water': 1.333
 'Clear Plastic': 1.400
 'Glass': 1.440 - 1.900
 'Light glass': 1.450
 'Standart glass': 1.520
 'Heavy glass': 1.650
 'Obsidian': 1.480 - 1.510
 'Onyx': 1.486 - 1.658
 'Acrylic glass': 1.491
 'Benzene': 1.501
 'Crown glass': 1.510
 'Jasper': 1.540
 'Agate': 1.544 - 1.553
 'Amethist': 1.544 - 1.553
 'Salt': 1.544
 'Amber': 1.550
 'Quartz': 1.550
 'Sugar': 1.560
 'Emerald': 1.576 - 1.582
 'Flint glass': 1.613
 'Topaz': 1.620 - 1.627
 'Jade': 1.660 - 1.680
 'Saphire': 1.760
 'Ruby': 1.760 - 2.419
 'Crystal': 1.870
 'Diamond': 2.417 - 2.541
 */

//Non-PBR blinn-phong material
struct material {
	char name[MATERIAL_NAME_SIZE];
	char textureFilename[OBJ_FILENAME_LENGTH];
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

enum bsdfType {
	emission = 0,
	diffuse,
	glass,
	glossy,
	refraction,
	translucent,
	transparent
};

//TODO: Different BSDF materials
struct BSDF {
	enum bsdfType *type;
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
