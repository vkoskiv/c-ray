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
#include "vec2.h"

struct lightRay;
struct intersection;
struct texture;

enum diffuseBSDF
{
	DIFFUSE_BSDF_LAMBERT,
	DIFFUSE_BSDF_OREN_NEYAR,
	DIFFUSE_BSDF_EARL_HAMMON_GGX,
};

enum specularBSDF
{
	SPECULAR_BSDF_PHONG,
	SPECULAR_BLINN_PHONG,
	SPECULAR_BSDF_ERIC_HEITZ_GGX_2018,
};

enum materialType
{
	MATERIAL_TYPE_EMISSIVE,
	MATERIAL_TYPE_DEFAULT,
	MATERIAL_TYPE_WARNING
};

enum materialType getMaterialType_FromStr(const char* str);
enum diffuseBSDF getDiffuseBSDF_FromStr(const char* str);
enum specularBSDF getSpecularBSDF_FromStr(const char* str);

struct materialBucket {
	bool is_used;
	void* value;
	char* key;
};

struct material {
	struct materialBucket* data;
	uint64_t size;
	enum materialType type;
	enum diffuseBSDF diffuseBSDF;
	enum specularBSDF specularBSDF;
	const char* name;
};

typedef vec3(*diffuseBSDF_Func)(struct material *p_mat, vec3 wo, vec3 wi);
typedef vec3(*specularBSDF_Func)(struct material *p_mat, vec3 V, vec3 *p_Li, pcg32_random_t *p_rng);

inline uint64_t hashDataToU64(uint8_t* data, size_t size) {
	static const uint64_t FNV_offset_basis = 14695981039346656037U;
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

struct material* newMaterial(enum materialType type);
void freeMaterial(struct material* self);

struct materialBucket* getMaterialBucketPtr(struct material *self, const char *key);
void setMaterialVec3(struct material *self, const char *key, vec3 value);
void setMaterialFloat(struct material *self, const char *key, float value);

void setMaterialPtr(struct material* self, const char* key, void* value);
void *getMaterialPtr(struct material* self, const char* key);

float getMaterialFloat(struct material *self, const char *key);
vec3 getMaterialVec3(struct material *self, const char *key);
bool doesMaterialValueExist(struct material *self, const char *id);

color getAlbedo(struct material* p_mat);
void setMaterialColor(struct material* self, const char* key, color value);
color getMaterialColor(struct material* self, const char* key);

float getSpecularity(struct material* p_mat, vec2 uv);

static const float INV_PI = 1.0f / PI;

//bool LightingFunc(struct intersection* isect, vec3* attenuation, struct lightRay* scattered, pcg32_random_t* rng);
color lightingFuncDiffuse(struct material *p_mat, vec2 uv, vec3 wo, vec3 wi);
color lightingFuncSpecular(struct material *p_mat, vec2 uv, vec3 V, vec3* p_Li, pcg32_random_t* p_rng);
