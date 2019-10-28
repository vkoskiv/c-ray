//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "material.h"

#include "../renderer/pathtrace.h"
#include "../datatypes/vertexbuffer.h"
#include "../datatypes/texture.h"

static const uint64_t MATERIAL_TABLE_SIZE = 32;
IMaterial NewMaterial(MaterialType type)
{
	Material* self = malloc(sizeof(Material));
	self->size = MATERIAL_TABLE_SIZE;
	self->data = malloc(sizeof(MaterialEntry) * MATERIAL_TABLE_SIZE);
	for (uint64_t i = 0; i < MATERIAL_TABLE_SIZE; ++i)
	{
		self->data[i].is_used = false;
		self->data[i].data = NULL;
	}

	self->type = type;
	return self;
}

void MaterialFree(IMaterial self)
{
	if (self == NULL) return;
	for (uint64_t i = 0; i < MATERIAL_TABLE_SIZE; ++i)
	{
		if(self->data[i].is_used) free(self->data[i].data);
	}
	free(self->data);
	free(self);
}

MaterialEntry* MaterialGetEntry(IMaterial self, const char* id)
{
	return &self->data[HashDataToU64(id, strlen(id)) % self->size];
}

void MaterialSetVec3(IMaterial self, const char* id, vec3 value)
{
	MaterialEntry* pentry = MaterialGetEntry(self, id);
	pentry->data = malloc(sizeof(vec3));
	pentry->is_used = true;
	*(vec3*)pentry->data = value;
}

void MaterialSetFloat(IMaterial self, const char* id, float value)
{
	MaterialEntry* pentry = MaterialGetEntry(self, id);
	pentry->data = malloc(sizeof(float));
	pentry->is_used = true;
	*(float*)pentry->data = value;
}

float MaterialGetFloat(IMaterial self, const char* id)
{
	return *(float*)MaterialGetEntry(self, id)->data;
}

vec3 MaterialGetVec3(IMaterial self, const char* id)
{
	MaterialEntry* pentry = MaterialGetEntry(self, id);
	if (pentry->is_used)
		return *(vec3*)pentry->data;
	else
		return (vec3) { 0.0f, 0.0f, 0.0f };
}

bool MaterialValueAt(IMaterial self, const char* id)
{
	return MaterialGetEntry(self, id)->is_used;
}


IMaterial MaterialPhongDefault(void)
{
	IMaterial mat = NewMaterial(MATERIAL_TYPE_DEFAULT);
	MaterialSetVec3(mat, "albedo", (vec3) { 0.5f, 0.5f, 0.5f });
	MaterialSetFloat(mat, "roughness", 0.5f);
	return mat;
}
/*
vec3 colorForUV(struct intersection* isect) {
	struct color output = { 0.0,0.0,0.0,0.0 };
	struct material mtl = isect->end;
	struct poly p = polygonArray[isect->polyIndex];

	//Texture width and height for this material
	float width = *mtl.texture->width;
	float heigh = *mtl.texture->height;

	//barycentric coordinates for this polygon
	float u = isect->uv.x;
	float v = isect->uv.y;
	float w = 1.0 - u - v;

	//Weighted texture coordinates
	struct coord ucomponent = coordScale(u, textureArray[p.textureIndex[1]]);
	struct coord vcomponent = coordScale(v, textureArray[p.textureIndex[2]]);
	struct coord wcomponent = coordScale(w, textureArray[p.textureIndex[0]]);

	// textureXY = u * v1tex + v * v2tex + w * v3tex
	struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);

	float x = (textureXY.x * (width));
	float y = (textureXY.y * (heigh));

	//Get the color value at these XY coordinates
	output = textureGetPixelFiltered(mtl.texture, x, y);

	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
	//FIXME: Maybe ask lodepng if we actually need to do this transform
	output = fromSRGB(output);

	return output;
}
*/
vec3 GetAlbedo(IMaterial mat)
{
	if (MaterialValueAt(mat, "albedo"))	return MaterialGetVec3(mat, "albedo");
	else return (vec3) { 1.0f, 0.0f, 1.0f };
}


float Square(float x) { return x * x; }
float Theta(vec3 w) { return acos(w.z / vec3_length(w)); }
float Phi(vec3 w) { return atan(w.y / w.x); }
float CosPhi(vec3 w) { return cos(Phi(w)); }
float CosTheta(vec3 w) { return cos(Theta(w)); }
float Cos2Theta(vec3 w) { return Square(CosTheta(w)); }
float AbsTanTheta(vec3 w) { return fabs(tan(Theta(w))); }
float AbsCosTheta(vec3 w) { return fabs(CosTheta(w)); }

vec3 LambertDiffuseBSDF(IMaterial mat, vec3 wo, vec3 wi)
{
	float NoL = CosTheta(wi);
	return vec3_muls(GetAlbedo(mat), max(NoL, 0.0) * INV_PI);
}

float HeitzGgxDGTR2Aniso(vec3 wm, float ax, float ay)
{
	float dotHX2 = Square(wm.x);
	float dotHY2 = Square(wm.y);
	float cos2Theta = Cos2Theta(wm);
	float ax2 = Square(ax);
	float ay2 = Square(ay);
	return 1.0f / (PI * ax * ay * Square(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta));
}

float HeitzGgxG1GTR2Aniso(vec3 wm, vec3 w, float ax, float ay)
{
	float NoX = vec3_dot(wm, w);

	if (NoX <= 0.0f) return 0.0f;

	float phi = Phi(w);
	float cos2Phi = Square(cos(phi));
	float sin2Phi = Square(sin(phi));
	float tanTheta = tan(Theta(w));

	float a = 1.0f / (tanTheta * sqrt(cos2Phi * ax * ax + sin2Phi * ay * ay));
	float a2 = a * a;

	float lambda = 0.5f * (-1.0f + sqrt(1.0f + 1.0f / a2));
	return 1.0f / (1.0f + lambda);
}

float HeitzGgxG2GTR2Aniso(vec3 wm, vec3 wo, vec3 wi, float ax, float ay)
{
	return HeitzGgxG1GTR2Aniso(wm, wo, ax, ay) * HeitzGgxG1GTR2Aniso(wm, wi, ax, ay);
}

float SchlickFresnel(float NoX, float F0)
{
	return F0 + (1.0f - F0) * pow(1.0f - NoX, 5.0f);
}

vec3 EricHeitzGgx2018(Material* mat, vec3 wo, vec3 wi)
{
	float NoV = CosTheta(wo);
	float NoL = CosTheta(wi);

	if (NoV <= 0.0f || NoL <= 0.0f) return VEC3_ZERO;

	vec3 albedo = MaterialGetVec3(mat, "albedo");
	float roughness = MaterialGetFloat(mat, "roughness");
	float anisotropy = MaterialGetFloat(mat, "anisotropy");
	float ior = MaterialGetFloat(mat, "ior");

	float alpha = roughness * roughness;
	float aspect = sqrt(1.0f - 0.9f * anisotropy);
	float ax = alpha * aspect;
	float ay = alpha / aspect;

	vec3 wm = vec3_normalize(vec3_add(wo, wi));
	float NoH = CosTheta(wm);
	float VoH = vec3_dot(wo, wm);

	float F0 = abs((1.0 - ior) / (1.0 + ior));

	float D = HeitzGgxDGTR2Aniso(wm, ax, ay);
	float F = SchlickFresnel(NoL, F0);
	float G = HeitzGgxG2GTR2Aniso(wm, wo, wi, ax, ay);

	float Fr = (D * F * G) / (4.0f * NoV * max(NoL, 0.0f) + 0.05f);

	return (vec3) { Fr, Fr, Fr };
}

vec3 LightingFuncDiffuse(IMaterial mat, vec3 wo, vec3 wi)
{
	switch (mat->diffuse_bsdf_type)
	{
		case BSDF_TYPE_LAMBERT_DIFFUSE:
		{
			return LambertDiffuseBSDF(mat, wo, wi);
		}
	}

	return (vec3) { 0.0f, 0.0f, 0.0f };
}

vec3 LightingFuncSpecular(IMaterial mat, vec3 wo, vec3 wi)
{
	switch (mat->specular_bsdf_type)
	{
		case BSDF_TYPE_ERIC_HEITZ_GGX_2018_SPECULAR:
		{
			return EricHeitzGgx2018(mat, wo, wi);
		}
	}

	return (vec3) { 0.0f, 0.0f, 0.0f };
}

//bool LightingFunc(struct intersection* isect, vec3* attenuation, struct lightRay* scattered, pcg32_random_t* rng)
//{
//	if (isect->end->type == MATERIAL_TYPE_EMISSIVE) return false;
//
//	switch (isect->end->diffuse_bsdf_type)
//	{
//	case BSDF_TYPE_LAMBERT_DIFFUSE:
//	{
//		vec3 temp = vecAdd(isect->hitPoint, isect->surfaceNormal);
//		vec3 rand = RandomUnitSphere(rng);
//		vec3 target = vecAdd(temp, rand);
//		vec3 target2 = vecNormalize(vecSubtract(isect->hitPoint, target));
//		*scattered = ((struct lightRay) { isect->hitPoint, target2, rayTypeScattered, isect->end, 0 });
//		*attenuation = GetAlbedo(isect->end);
//
//		return true;
//	}
//	}
//
//	return false;
//}


//
//
//
////FIXME: Temporary, eventually support full OBJ spec
//struct material newMaterial(struct color diffuse, float reflectivity) {
//	struct material newMaterial = {0};
//	newMaterial.reflectivity = reflectivity;
//	newMaterial.diffuse = diffuse;
//	return newMaterial;
//}
//
//
//struct material newMaterialFull(struct color ambient,
//								struct color diffuse,
//								struct color specular,
//								float reflectivity,
//								float refractivity,
//								float IOR,
//								float transparency,
//								float sharpness,
//								float glossiness) {
//	struct material mat;
//	
//	mat.ambient = ambient;
//	mat.diffuse = diffuse;
//	mat.specular = specular;
//	mat.reflectivity = reflectivity;
//	mat.refractivity = refractivity;
//	mat.IOR = IOR;
//	mat.transparency = transparency;
//	mat.sharpness = sharpness;
//	mat.glossiness = glossiness;
//	
//	return mat;
//}
//
//struct material emptyMaterial() {
//	return (struct material){0};
//}
//
//struct material defaultMaterial() {
//	struct material newMat = emptyMaterial();
//	newMat.diffuse = grayColor;
//	newMat.reflectivity = 1.0;
//	newMat.type = lambertian;
//	newMat.IOR = 1.0;
//	return newMat;
//}
//
////To showcase missing .MTL file, for example
//struct material warningMaterial() {
//	struct material newMat = emptyMaterial();
//	newMat.type = lambertian;
//	newMat.diffuse = (struct color){1.0, 0.0, 0.5, 0.0};
//	return newMat;
//}
//
////Find material with a given name and return a pointer to it
//struct material *materialForName(struct material *materials, int count, char *name) {
//	for (int i = 0; i < count; i++) {
//		if (strcmp(materials[i].name, name) == 0) {
//			return &materials[i];
//		}
//	}
//	return NULL;
//}
//
//void assignBSDF(struct material *mat) {
//	//TODO: Add the BSDF weighting here
//	switch (mat->type) {
//		case lambertian:
//			mat->bsdf = lambertianBSDF;
//			break;
//		case metal:
//			mat->bsdf = metallicBSDF;
//			break;
//		case emission:
//			mat->bsdf = emissiveBSDF;
//			break;
//		case glass:
//			mat->bsdf = dialectricBSDF;
//			break;
//		default:
//			mat->bsdf = lambertianBSDF;
//			break;
//	}
//}
//
////Transform the intersection coordinates to the texture coordinate space
////And grab the color at that point. Texture mapping.
//struct color colorForUV(struct intersection *isect) {
//	struct color output = {0.0,0.0,0.0,0.0};
//	struct material mtl = isect->end;
//	struct poly p = polygonArray[isect->polyIndex];
//	
//	//Texture width and height for this material
//	float width = *mtl.texture->width;
//	float heigh = *mtl.texture->height;
//	
//	//barycentric coordinates for this polygon
//	float u = isect->uv.x;
//	float v = isect->uv.y;
//	float w = 1.0 - u - v;
//	
//	//Weighted texture coordinates
//	struct coord ucomponent = coordScale(u, textureArray[p.textureIndex[1]]);
//	struct coord vcomponent = coordScale(v, textureArray[p.textureIndex[2]]);
//	struct coord wcomponent = coordScale(w, textureArray[p.textureIndex[0]]);
//	
//	// textureXY = u * v1tex + v * v2tex + w * v3tex
//	struct coord textureXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
//	
//	float x = (textureXY.x*(width));
//	float y = (textureXY.y*(heigh));
//	
//	//Get the color value at these XY coordinates
//	output = textureGetPixelFiltered(mtl.texture, x, y);
//	
//	//Since the texture is probably srgb, transform it back to linear colorspace for rendering
//	//FIXME: Maybe ask lodepng if we actually need to do this transform
//	output = fromSRGB(output);
//	
//	return output;
//}
//
//struct color gradient(struct intersection *isect) {
//	//barycentric coordinates for this polygon
//	float u = isect->uv.x;
//	float v = isect->uv.y;
//	float w = 1.0 - u - v;
//	
//	return colorWithValues(u, v, w, 1.0);
//}
//
////FIXME: Make this configurable
////This is a checkerboard pattern mapped to the surface coordinate space
//struct color mappedCheckerBoard(struct intersection *isect, float coef) {
//	struct poly p = polygonArray[isect->polyIndex];
//	
//	//barycentric coordinates for this polygon
//	float u = isect->uv.x;
//	float v = isect->uv.y;
//	float w = 1.0 - u - v; //1.0 - u - v
//	
//	//Weighted coordinates
//	struct coord ucomponent = coordScale(u, textureArray[p.textureIndex[1]]);
//	struct coord vcomponent = coordScale(v, textureArray[p.textureIndex[2]]);
//	struct coord wcomponent = coordScale(w,	textureArray[p.textureIndex[0]]);
//	
//	// textureXY = u * v1tex + v * v2tex + w * v3tex
//	struct coord surfaceXY = addCoords(addCoords(ucomponent, vcomponent), wcomponent);
//	
//	float sines = sin(coef*surfaceXY.x) * sin(coef*surfaceXY.y);
//	
//	if (sines < 0) {
//		return (struct color){0.4, 0.4, 0.4, 0.0};
//	} else {
//		return (struct color){1.0, 1.0, 1.0, 0.0};
//	}
//}
//
////FIXME: Make this configurable
////This is a spatial checkerboard, mapped to the world coordinate space (always axis aligned)
//struct color checkerBoard(struct intersection *isect, float coef) {
//	float sines = sin(coef*isect->hitPoint.x) * sin(coef*isect->hitPoint.y) * sin(coef*isect->hitPoint.z);
//	if (sines < 0) {
//		return (struct color){0.4, 0.4, 0.4, 0.0};
//	} else {
//		return (struct color){1.0, 1.0, 1.0, 0.0};
//	}
//}
//
///**
// Compute reflection vector from a given vector and surface normal
// 
// @param vec Incident ray to reflect
// @param normal Surface normal at point of reflection
// @return Reflected vector
// */
//vec3 reflectVec(const vec3 *incident, const vec3 *normal) {
//	float reflect = 2.0 * vecDot(*incident, *normal);
//	return vecSubtract(*incident, vecScale(reflect, *normal));
//}
//
//vec3 randomInUnitSphere(pcg32_random_t *rng) {
//	vec3 vec = (vec3){0.0, 0.0, 0.0};
//	do {
//		vec = vecMultiplyConst(vecWithPos(rndFloat(0, 1, rng), rndFloat(0, 1, rng), rndFloat(0, 1, rng)), 2.0);
//		vec = vecSubtract(vec, vecWithPos(1.0, 1.0, 1.0));
//	} while (vecLengthSquared(vec) >= 1.0);
//	return vec;
//}
//
//vec3 randomOnUnitSphere(pcg32_random_t *rng) {
//	vec3 vec = (vec3){0.0, 0.0, 0.0};
//	do {
//		vec = vecMultiplyConst(vecWithPos(rndFloat(0, 1, rng), rndFloat(0, 1, rng), rndFloat(0, 1, rng)), 2.0);
//		vec = vecSubtract(vec, vecWithPos(1.0, 1.0, 1.0));
//	} while (vecLengthSquared(vec) >= 1.0);
//	return vecNormalize(vec);
//}
//
//bool emissiveBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
//	return false;
//}
//
//bool weightedBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
//	
//	/*
//	 This will be the internal shader weighting solver that runs a random distribution and chooses from the available
//	 discrete shaders.
//	 */
//	
//	return false;
//}
//
////TODO: Make this a function ptr in the material?
//struct color diffuseColor(struct intersection *isect) {
//	if (isect->end.hasTexture) {
//		return colorForUV(isect);
//	} else {
//		return isect->end.diffuse;
//	}
//}
//
//bool lambertianBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
//	vec3 temp = vecAdd(isect->hitPoint, isect->surfaceNormal);
//	vec3 rand = randomInUnitSphere(rng);
//	vec3 target = vecAdd(temp, rand);
//	vec3 target2 = vecSubtract(isect->hitPoint, target);
//	*scattered = ((struct lightRay){isect->hitPoint, target2, rayTypeScattered, isect->end, 0});
//	*attenuation = diffuseColor(isect);
//	return true;
//}
//
//bool metallicBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
//	vec3 normalizedDir = vecNormalize(isect->ray.direction);
//	vec3 reflected = reflectVec(&normalizedDir, &isect->surfaceNormal);
//	//Roughness
//	if (isect->end.roughness > 0.0) {
//		vec3 fuzz = vecMultiplyConst(randomInUnitSphere(rng), isect->end.roughness);
//		reflected = vecAdd(reflected, fuzz);
//	}
//	
//	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
//	*attenuation = diffuseColor(isect);
//	return (vecDot(scattered->direction, isect->surfaceNormal) > 0);
//}
//
//bool refract(vec3 in, vec3 normal, float niOverNt, vec3 *refracted) {
//	vec3 uv = vecNormalize(in);
//	float dt = vecDot(uv, normal);
//	float discriminant = 1.0 - niOverNt * niOverNt * (1 - dt * dt);
//	if (discriminant > 0) {
//		vec3 A = vecMultiplyConst(normal, dt);
//		vec3 B = vecSubtract(uv, A);
//		vec3 C = vecMultiplyConst(B, niOverNt);
//		vec3 D = vecMultiplyConst(normal, sqrt(discriminant));
//		*refracted = vecSubtract(C, D);
//		return true;
//	} else {
//		return false;
//	}
//}
//
//float shlick(float cosine, float IOR) {
//	float r0 = (1 - IOR) / (1 + IOR);
//	r0 = r0*r0;
//	return r0 + (1 - r0) * pow((1 - cosine), 5);
//}
//
//// Only works on spheres for now. Reflections work but refractions don't
//bool dialectricBSDF(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered, pcg32_random_t *rng) {
//	vec3 outwardNormal;
//	vec3 reflected = reflectVec(&isect->ray.direction, &isect->surfaceNormal);
//	float niOverNt;
//	*attenuation = diffuseColor(isect);
//	vec3 refracted;
//	float reflectionProbability;
//	float cosine;
//	
//	if (vecDot(isect->ray.direction, isect->surfaceNormal) > 0) {
//		outwardNormal = vecNegate(isect->surfaceNormal);
//		niOverNt = isect->end.IOR;
//		cosine = isect->end.IOR * vecDot(isect->ray.direction, isect->surfaceNormal) / vecLength(isect->ray.direction);
//	} else {
//		outwardNormal = isect->surfaceNormal;
//		niOverNt = 1.0 / isect->end.IOR;
//		cosine = -(vecDot(isect->ray.direction, isect->surfaceNormal) / vecLength(isect->ray.direction));
//	}
//	
//	if (refract(isect->ray.direction, outwardNormal, niOverNt, &refracted)) {
//		reflectionProbability = shlick(cosine, isect->end.IOR);
//	} else {
//		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
//		reflectionProbability = 1.0;
//	}
//	
//	//Roughness
//	if (isect->end.roughness > 0.0) {
//		vec3 fuzz = vecMultiplyConst(randomInUnitSphere(rng), isect->end.roughness);
//		reflected = vecAdd(reflected, fuzz);
//		refracted = vecAdd(refracted, fuzz);
//	}
//	
//	if (rndFloat(0, 1, rng) < reflectionProbability) {
//		*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
//	} else {
//		*scattered = newRay(isect->hitPoint, refracted, rayTypeRefracted);
//	}
//	return true;
//}
//
//void freeMaterial(struct material *mat) {
//	if (mat->textureFilePath) {
//		free(mat->textureFilePath);
//	}
//	if (mat->name) {
//		free(mat->name);
//	}
//}
