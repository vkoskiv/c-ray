//
//  material.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 20/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "material.h"

#define INV_PI (1.0/PI)

float Square(float x) {	return x * x; }
float Theta(vec3 w) { return acos(w.z / vec3_length(w)); }
float Phi(vec3 w) { return atan(w.y / w.x); }
float CosPhi(vec3 w) { return cos(Phi(w)); }
float CosTheta(vec3 w) { return cos(Theta(w)); }
float Cos2Theta(vec3 w) { return Square(CosTheta(w)); }
float AbsTanTheta(vec3 w) { return fabs(tan(Theta(w))); }
float AbsCosTheta(vec3 w) { return fabs(CosTheta(w)); }

float mix(float a, float b, float t)
{
	return t * a + (1.0f - t) * b;
}

/*
	PBR Diffuse Lighting for GGX + Smith Microsurfaces by Earl Hammon, Jr (https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf)
*/

vec3 HammonDiffuseBSDF(Material* mat, vec3 wo, vec3 wi)
{
	float NoV = CosTheta(wo);
	float NoL = CosTheta(wi);

	if (NoV <= 0.0f || NoL <= 0.0f) return VEC3_ZERO;

	vec3 wm = vec3_normalize(vec3_add(wo, wi)); /* Half vector */

	float NoH = CosTheta(wm);
	float VoL = vec3_dot(wo, wi);

	float alpha = mat->roughness * mat->roughness;

	float facing = 0.5f + 0.5f * VoL;
	float roughy = facing * (0.9f - 0.4f * facing) * (0.5f + NoH)/NoH;
	float smoothy = 1.05f * (1.0f - pow(1.0f - NoL, 5.0f)) * (1.0f - pow(1.0f - NoV, 5.0f));
	float single = INV_PI * mix(smoothy, roughy, alpha);
	float multi = alpha * 0.1159f;
	return vec3_mul(mat->albedo, vec3_adds(vec3_muls(mat->albedo, multi), single) );
}


/*
	Sampling the GGX Distribution of Visible Normals by Eric Heitz (http://jcgt.org/published/0007/04/01/paper.pdf)
*/
float HeitzGgxDGTR2(vec3 wm, float ax, float ay)
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

	float a = 1.0f / (tanTheta * sqrtf(cos2Phi * ax * ax + sin2Phi * ay * ay));
    float a2 = a * a;

    float lambda = 0.5f * (-1.0f + sqrtf(1.0f + 1.0f / a2));
    return 1.0f / (1.0f + lambda);
}

float HeitzGgxG2GTR2Aniso(vec3 wm, vec3 wo, vec3 wi, float ax, float ay)
{
	return HeitzGgxG1GTR2Aniso(wm, wo, ax, ay) * HeitzGgxG1GTR2Aniso(wm, wi, ax, ay);
}

float SchlickFresnel(float NoX, float F0)
{
  return F0 + (1.0f - F0) * powf(1.0f - NoX, 5.0f);
}

vec3 HeitzSpecularBSDF(Material* mat, vec3 wo, vec3 wi)
{
	float NoV = CosTheta(wo);
	float NoL = CosTheta(wi);

	if (NoV <= 0.0f || NoL <= 0.0f) return VEC3_ZERO;

	vec3 wm = vec3_normalize(vec3_add(wo, wi)); /* Half vector */
	float VoH = vec3_dot(wo, wm);

	float alpha = mat->roughness * mat->roughness; /* GGX alpha */
	float aspect = sqrtf(1.0f - 0.9f * mat->anisotropy);
	float ax = alpha * aspect;
	float ay = alpha / aspect;

	/* Calculate GGX visibility and distribution */
	float G = HeitzGgxG2GTR2Aniso(wm, wo, wi, ax, ay);
	float D = HeitzGgxDGTR2(wm, ax, ay);

	/* Calculate fresnel response at zero degrees, then the Francois Schlick fresnel term */
	float F0 = Square(fabs( (1.0 - mat->ior) / (1.0 + mat->ior) ));
	float F = SchlickFresnel(VoH, F0);

	float L = (D * F * G) / (4.0f * NoV * max(NoL, 0.0f) + 0.05f);

	return vec3_new(L, L, L);
}