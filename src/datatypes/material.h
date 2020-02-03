//
//  material.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "color.h"

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

struct lightRay;
struct hitRecord;

enum bsdfType {
	emission = 0,
	lambertian,
	glass,
	metal,
	translucent,
	transparent
};

struct bsdf {
	enum bsdfType type;
	float weights;
	bool (*bsdf)(struct hitRecord*, struct lightRay*, struct color*, struct lightRay*, pcg32_random_t*);
};

struct material {
	char *textureFilePath;
	char *normalMapPath;
	char *name;
	bool hasTexture;
	struct texture *texture;
	bool hasNormalMap;
	struct texture *normalMap;
	struct color ambient;
	struct color diffuse;
	struct color specular;
	struct color emission;
	float reflectivity;
	float roughness;
	float refractivity;
	float IOR;
	float transparency;
	float sharpness;
	float glossiness;
	
	//TODO:
	// - New BSDF struct that contains FP, type and contributing percentage
	// - Have an array of these structs, so 50% emit 50% reflect would have
	//   two BSDF structs that have their contributions set to 0.5
	// - Modify pathTrace loop to accommodate this new system.
	// - Normalize probabilities
	
	enum bsdfType type;
	//isect record, ray, attenuation color, scattered ray, rng
	bool (*bsdf)(struct hitRecord*, const struct lightRay*, struct color*, struct lightRay*, pcg32_random_t*);
};

//temporary newMaterial func
struct material newMaterial(struct color diffuse, float reflectivity);
struct material *materialForName(struct material *materials, int count, char *name);

//Full obj spec material
struct material newMaterialFull(struct color ambient,
								struct color diffuse,
								struct color specular,
								float reflectivity,
								float refractivity,
								float IOR,
								float transparency,
								float sharpness,
								float glossiness);

struct material emptyMaterial(void);
struct material defaultMaterial(void);
struct material warningMaterial(void);

bool emissiveBSDF(struct hitRecord *isect, const struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng);
bool lambertianBSDF(struct hitRecord *isect, const struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng);
bool metallicBSDF(struct hitRecord *isect, const struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng);
bool dialectricBSDF(struct hitRecord *isect, const struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng);

void assignBSDF(struct material *mat);

void destroyMaterial(struct material *mat);
