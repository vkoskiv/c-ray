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

#include "vec3.h"

struct lightRay;
struct intersection;
struct texture;

typedef enum
{
	BSDF_TYPE_LAMBERT_DIFFUSE,
	BSDF_TYPE_ERIC_HEITZ_GGX_2018_SPECULAR,
	BSDF_TYPE_PHONG_SPECULAR,
	BSDF_TYPE_PHONG_DIFFUSE,
	BSDF_TYPE_BLINN_PHONG_DIFFUSE,
	BSDF_TYPE_BLINN_PHONG_SPECULAR,
	BSDF_TYPE_EARL_HAMMON_GGX_DIFFUSE,
	BSDF_TYPE_OREN_NEYAR_DIFFUSE
} BSDF_TYPE;

static BSDF_TYPE ValueStrToDiffBSDF(const char* str)
{
	BSDF_TYPE type = BSDF_TYPE_LAMBERT_DIFFUSE;

	if      (!strcmp(str, "Lambert"))         type = BSDF_TYPE_LAMBERT_DIFFUSE;
	else if (!strcmp(str, "Phong"))           type = BSDF_TYPE_PHONG_DIFFUSE;
	else if (!strcmp(str, "Blinn-Phong"))     type = BSDF_TYPE_BLINN_PHONG_DIFFUSE;
	else if (!strcmp(str, "Oren Neyar"))      type = BSDF_TYPE_OREN_NEYAR_DIFFUSE;
	else if (!strcmp(str, "Earl Hammon GGX")) type = BSDF_TYPE_EARL_HAMMON_GGX_DIFFUSE;

	return type;
}

static BSDF_TYPE ValueStrToSpecBSDF(const char* str)
{
	BSDF_TYPE type = BSDF_TYPE_PHONG_SPECULAR;

	if      (!strcmp(str, "Phong"))               type = BSDF_TYPE_PHONG_SPECULAR;
	else if (!strcmp(str, "Blinn-Phong"))         type = BSDF_TYPE_BLINN_PHONG_SPECULAR;
	else if (!strcmp(str, "Eric Heitz GGX 2018")) type = BSDF_TYPE_ERIC_HEITZ_GGX_2018_SPECULAR;

	return type;
}

typedef enum 
{
	MATERIAL_TYPE_EMISSIVE,
	MATERIAL_TYPE_DEFAULT,
	MATERIAL_TYPE_WARNING
} MaterialType;


static MaterialType ValueStrToMaterialType(const char* str)
{
	MaterialType type = MATERIAL_TYPE_DEFAULT;

	if (!strcmp(str, "Emissive")) type = MATERIAL_TYPE_EMISSIVE;

	return type;
}

typedef enum
{
	MATERIAL_VAR_TYPE_SCALAR,
	MATERIAL_VAR_TYPE_VEC3,
} MaterialVarType;

typedef struct
{
	bool is_used;
	void* data;
} MaterialEntry;

typedef struct
{
	MaterialEntry* data;
	uint64_t size;
	MaterialType type;
	BSDF_TYPE diffuse_bsdf_type;
	BSDF_TYPE specular_bsdf_type;
	const char* name;
} Material, *IMaterial;

typedef vec3(*BSDF_PFN)(IMaterial mat, vec3 wo, vec3 wi);


inline uint64_t HashDataToU64(uint8_t* data, size_t size)
{
	static const uint64_t FNV_offset_basis = 14695981039346656037;
	static const uint64_t FNV_prime = 1099511628211;

	uint64_t hash = FNV_offset_basis;

	for (size_t i = 0; i < size; ++i)
	{
		uint8_t byte_of_data = data[i];
		hash = (~0xFF & hash) | (0xFF & (hash ^ byte_of_data));
		hash = hash * FNV_prime;
	}

	return hash;
}

IMaterial NewMaterial(MaterialType type);
void MaterialFree(IMaterial self);

MaterialEntry* MaterialGetEntry(IMaterial self, const char* id);
void MaterialSetVec3(IMaterial self, const char* id, vec3 value);
void MaterialSetFloat(IMaterial self, const char* id, float value);
float MaterialGetFloat(IMaterial self, const char* id);
vec3 MaterialGetVec3(IMaterial self, const char* id);
bool MaterialValueAt(IMaterial self, const char* id);
IMaterial MaterialPhongDefault(void);

vec3 GetAlbedo(IMaterial mat);

/* Default BSDF function decl */
static const float INV_PI = 1.0f / PI;
vec3 LambertDiffuseBSDF(IMaterial mat, vec3 wo, vec3 wi);

typedef struct
{
	BSDF_TYPE type;
	BSDF_PFN pfn;
} BSDF;

//bool LightingFunc(struct intersection* isect, vec3* attenuation, struct lightRay* scattered, pcg32_random_t* rng);
vec3 LightingFuncDiffuse(IMaterial mat, vec3 wo, vec3 wi);
vec3 LightingFuncSpecular(Material* p_mat, vec3 V, vec3* p_Li, pcg32_random_t* p_rng);