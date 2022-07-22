//
//  material.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "color.h"
#include "../renderer/samplers/sampler.h"

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
struct color;

enum bsdfType {
	emission = 0,
	lambertian,
	glass,
	plastic,
	metal,
	translucent,
	transparent
};

struct material {
	char *name;
	struct texture *texture;
	struct texture *normalMap;
	struct texture *specularMap;
	struct color ambient;
	struct color diffuse;
	struct color specular;
	struct color emission;
	int illum;
	float shinyness;
	float reflectivity;
	float roughness;
	float refractivity;
	float IOR;
	float transparency;
	float sharpness;
	float glossiness;
	
	enum bsdfType type; // FIXME: Temporary
	const struct bsdfNode *bsdf;
};

struct material defaultMaterial(void);
struct material warningMaterial(void);

struct node_storage;

void try_to_guess_bsdf(const struct node_storage *s, struct material *mat);

#include "../utils/mempool.h"
#include "../nodes/valuenode.h" //FIXME: Remove
#include "../nodes/colornode.h" //FIXME: Remove

void destroyMaterial(struct material *mat);
