//
//  pathtrace.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "pathtrace.h"

#include "../datatypes/scene.h"
#include "../datatypes/camera.h"
#include "../acceleration/bbox.h"
#include "../acceleration/kdtree.h"
#include "../datatypes/texture.h"

#include "../datatypes/mat3.h"

bool getHit(struct intersection* rec, struct lightRay *incidentRay, struct world *scene);
color getBackground(struct lightRay *incidentRay, struct world *scene);

vec3 RandomUnitSphere(pcg32_random_t* rng) {
	vec3 vec = (vec3){ 0.0f, 0.0f, 0.0f };
	do {
		vec = (vec3){ rndFloat(0.0f, 1.0f, rng), rndFloat(0.0f, 1.0f, rng), rndFloat(0.0f, 1.0f, rng) };
		vec = vec3_subs(vec3_muls(vec, 2.0f), 1.0f);
	} while (vecLengthSquared(vec) >= 1.0f);
	return vec;
}
const float EPSILON = 0.00001f;

bool IsPureSpec(struct material *p_mat)
{
	float specularity = getMaterialFloat(p_mat, "specularity");
	return specularity >= 1.0f;
}

bool IsPureDiff(struct material *p_mat)
{
	float specularity = getMaterialFloat(p_mat, "specularity");
	return specularity <= 0.0f;
}

extern vec2* textureArray;

color pathTrace(struct lightRay *incidentRay, struct world *scene, int maxDepth, pcg32_random_t *p_rng, bool *hasHitObject) {

	color col = blackColor;
	color falloff = whiteColor;

	for (int i = 0; i < maxDepth; ++i) {
		struct intersection isect;

		if (getHit(&isect, incidentRay, scene)) {
			vec3 wo, wi;
			if (hasHitObject) *hasHitObject = true;
			wo = vec3_negate(incidentRay->direction); // view direction from hitpoint

			if (isect.end->type == MATERIAL_TYPE_EMISSIVE) {
				col = getAlbedo(isect.end);
				break;
			}
			else if (isect.end->type == MATERIAL_TYPE_DEFAULT) {
				struct material* p_mat = isect.end;
				float specularity = getMaterialFloat(p_mat, "specularity");
				float metalness = getMaterialFloat(p_mat, "metalness");

				// Setup TBN matrix for tangent space transforms
				vec3 N = isect.surfaceNormal;
				vec3 T = vec3_normalize(vec3_cross((vec3) { N.y, N.x, N.z }, N)); // to prevent cross product being length 0
				vec3 B = vec3_cross(N, T);

				mat3 TBN = (mat3){.v = {T, B, N}};
				mat3 invTBN = mat3_transpose(TBN);

				wo = vec3_normalize(mat3_mul_vec3(invTBN, wo));

				vec2 uv = (vec2){ 0.0f, 0.0f };
				if (isect.type == hitTypePolygon)
				{
					// polygon
					struct poly p = polygonArray[isect.polyIndex];

					// barycentric coordinates for this polygon
					float uf = isect.uv.x;
					float vf = isect.uv.y;
					float wf = 1.0f - uf - vf;

					// Weighted texture coordinates
					vec2 u = coordScale(uf, textureArray[p.textureIndex[1]]);
					vec2 v = coordScale(vf, textureArray[p.textureIndex[2]]);
					vec2 w = coordScale(wf, textureArray[p.textureIndex[0]]);

					uv = addCoords(addCoords(u, v), w);
				}				

				float U1 = rndFloat(0.0f, 1.0f, p_rng);
				bool isPureDiff = IsPureDiff(p_mat);
				bool isPureSpec = IsPureSpec(p_mat);

				float reflProb = 1.0f - (1.0f - specularity) * (1.0f - metalness);

				if (U1 < reflProb) {

					//falloff = vec3_mul(falloff, vec3_muls(diffuse, 1.0f - specularity));
					incidentRay->start = vec3_add(isect.hitPoint, vec3_muls(isect.surfaceNormal, EPSILON));

					// MIS sample using VNDF or NDF
					color specular = lightingFuncSpecular(p_mat, uv, wo, &wi, p_rng);

					incidentRay->direction = vec3_normalize(mat3_mul_vec3(TBN, wi));

					falloff = multiplyColors(falloff, specular);

				} else {

					// Light incoming direction from hitpoint, random in normal direction
					wi = vec3_normalize(vec3_add(RandomUnitSphere(p_rng), (vec3) { 0.0f, 0.0f, 1.0f }));

					incidentRay->start = vec3_add(isect.hitPoint, vec3_muls(N, EPSILON));
					incidentRay->direction = vec3_normalize(mat3_mul_vec3(TBN, wi));

					color diffuse = lightingFuncDiffuse(p_mat, uv, wo, wi);

					falloff = multiplyColors(falloff, diffuse);
				}
			}
		} else {
			col = getBackground(incidentRay, scene, i);
			break;
		}
	}

	return multiplyColors(col, falloff);
}

//vec3 pathTrace(struct lightRay* incidentRay, struct world* scene, int depth, int maxDepth, pcg32_random_t* rng, bool* hasHitObject) {
//	struct intersection isect = getClosestIsect(incidentRay, scene);
//	if (isect.didIntersect) {
//		if (hasHitObject) *hasHitObject = true;
//		struct lightRay scattered;
//		vec3 attenuation;
//
//		IMaterial mat = isect.end;
//		vec3 emitted = (mat->type == MATERIAL_TYPE_EMISSIVE && MaterialValueAt(mat, "albedo")) ? MaterialGetVec3(mat, "albedo") : VEC3_ZERO;
//
//		if (depth < maxDepth && LightingFunc(&isect, &attenuation, &scattered, rng)) {
//			float probability = 1;
//			if (depth >= 2) {
//				probability = max(attenuation.r, max(attenuation.g, attenuation.b));
//				if (rndFloat(0, 1, rng) > probability) {
//					return emitted;
//				}
//			}
//			vec3 newColor = pathTrace(&scattered, scene, depth + 1, maxDepth, rng, hasHitObject);
//			return vec3_muls(vec3_add(emitted, vec3_mul(attenuation, newColor)), 1.0 / probability);
//		}
//		else {
//			return emitted;
//		}
//	}
//	else {
//		return getBackground(incidentRay, scene);
//	}
//}

/**
 Calculate the closest intersection point, and other relevant information based on a given lightRay and scene
 See the intersection struct for documentation of what this function calculates.

 @param incidentRay Given light ray (set up in renderThread())
 @param scene  Given scene to cast that ray into
 @return intersection struct with the appropriate values set
 */
bool getHit(struct intersection *rec, struct lightRay *incidentRay, struct world *scene) {
	rec->distance = 20000.0;
	rec->ray = *incidentRay;
	rec->start = incidentRay->currentMedium;
	rec->didIntersect = false;
	for (int i = 0; i < scene->sphereCount; i++) {
		if (rayIntersectsWithSphere(&scene->spheres[i], incidentRay, rec)) {
			rec->end = scene->spheres[i].material;
			rec->didIntersect = true;
		}
	}
	for (int o = 0; o < scene->meshCount; o++) {
		if (rayIntersectsWithNode(scene->meshes[o].tree, incidentRay, rec)) {
			rec->end = scene->meshes[o].material;
			rec->didIntersect = true;
		}
	}
	return rec->didIntersect;
}

float wrapMax(float x, float max) {
	return fmod(max + fmod(x, max), max);
}

float wrapMinMax(float x, float min, float max) {
	return min + wrapMax(x - min, max - min);
}

color getHDRI(struct lightRay *incidentRay, struct world *scene, int index) {
	//Unit direction vector
	vec3 ud = vecNormalize(incidentRay->direction);
	
	//To polar from cartesian
	float r = 1.0f; //Normalized above
	float phi = (atan2f(ud.z, ud.x)/4) + scene->hdr->offset;
	float theta = acosf((-ud.y/r));
	
	float u = theta / PI;
	float v = (phi / (PI/2));
	
	u = wrapMinMax(u, 0, 1);
	v = wrapMinMax(v, 0, 1);
	
	float x =  (v * *scene->hdr->width);
	float y = (u * *scene->hdr->height);
	
	color newColor = textureGetPixelFiltered(!index ? scene->hdrBlurred : scene->hdr, x, y);
	
	return newColor;
}

//Linearly interpolate based on the Y component
color getAmbientColor(struct lightRay *incidentRay, struct gradient *c) {
	vec3 unitDirection = vecNormalize(incidentRay->direction);
	float t = 0.5 * (unitDirection.y + 1.0);
	return addColors(colorCoef((*c).down, 1.0 - t), colorCoef((*c).up, t));
}

color getBackground(struct lightRay *incidentRay, struct world *scene, int index) {
	return scene->hdr ? getHDRI(incidentRay, scene, index) : getAmbientColor(incidentRay, scene->ambientColor);
}
