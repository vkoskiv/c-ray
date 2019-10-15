//
//  material.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
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

#include <math.h>

typedef struct
{
	float x, y, z;
} vec3;

#define VEC3_ZERO ((vec3){0.0f, 0.0f, 0.0f})
#define VEC3_ONE ((vec3){1.0f, 1.0f, 1.0f})

vec3 vec3_new(float x, float y, float z)
{
	return (vec3){x, y, z};
}

float vec3_dot(vec3 a, vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float vec3_length(vec3 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

vec3 vec3_divs(vec3 v, float s)
{
	return (vec3){v.x / s, v.y / s, v.z / s};
}

vec3 vec3_normalize(vec3 v)
{
	return vec3_divs(v, vec3_length(v));
}

vec3 vec3_add(vec3 a, vec3 b)
{
	return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 vec3_sub(vec3 a, vec3 b)
{
	return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 vec3_mul(vec3 a, vec3 b)
{
	return (vec3){a.x * b.x, a.y * b.y, a.z * b.z};
}

vec3 vec3_adds(vec3 v, float s)
{
	return (vec3){v.x + s, v.y + s, v.z + s};
}

vec3 vec3_muls(vec3 v, float s)
{
	return (vec3){v.x * s, v.y * s, v.z * s};
}

typedef struct
{
	vec3 albedo;
	float metalness;
	float specularity;
	float ior;
	float anisotropy;
	float roughness;
} Material;


typedef enum
{
	BSDF_TYPE_DIFFUSE,
	BSDF_TYPE_SPECULAR,
} BSDF_TYPE;

typedef vec3 (*BSDF_FUN)(Material* mat, vec3 wo, vec3 wi);

typedef struct
{
	BSDF_TYPE type;
	BSDF_FUN fun;
} BSDF;

/* BSDF declarations */
vec3 HammonDiffuseBSDF(Material* mat, vec3 wo, vec3 wi);
vec3 HeitzSpecularBSDF(Material* mat, vec3 wo, vec3 wi);

enum BSDFLookupTableEnum
{
	HAMMON_DIFFUSE_BSDF,
	HEITZ_SPECULAR_BSDF,
	BSDF_LOOKUP_TABLE_SIZE
};

BSDF BSDFLookUpTable[BSDF_LOOKUP_TABLE_SIZE];

static void SetupBSDFLUT(void)
{
	BSDFLookUpTable[HAMMON_DIFFUSE_BSDF] = (BSDF){ BSDF_TYPE_DIFFUSE, (BSDF_FUN) &HammonDiffuseBSDF };
	BSDFLookUpTable[HEITZ_SPECULAR_BSDF] = (BSDF){ BSDF_TYPE_SPECULAR, (BSDF_FUN) &HeitzSpecularBSDF };
}

