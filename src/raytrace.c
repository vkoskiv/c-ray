//
//  raytrace.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "raytrace.h"

#include "scene.h"
#include "camera.h"
#include "poly.h"
#include "light.h"
#include "obj.h"
#include "bbox.h"
#include "kdtree.h"

struct intersection {
	struct lightRay *ray;
	struct material *start;
	struct material *end;
	struct vector *hitPoint;
	struct vector *surfaceNormal;
	bool didIntersect;
	double distance;
};


/**
 Traverse a k-d tree and see if a ray collides with a polygon.

 @param node Given tree to traverse
 @param ray Ray to check intersection on
 @param info Shading information
 @return True if ray hits a polygon in a leaf node, otherwise false
 */
bool rayIntersectsWithNode(struct kdTreeNode *node, struct lightRay *ray, struct shadeInfo *info) {
	if (rayIntersectWithAABB(node->bbox, ray, &info->closestIntersection)) {
		struct vector normal;
		struct coord uv;
		bool hasHit = false;
		
		if (node->left->polyCount > 0 || node->right->polyCount > 0) {
			//Recurse down both sides
			bool hitLeft  = rayIntersectsWithNode(node->left, ray, info);
			bool hitRight = rayIntersectsWithNode(node->right, ray, info);
			
			return hitLeft || hitRight;
		} else {
			//This is a leaf, so check all polys
			for (int i = 0; i < node->polyCount; i++) {
				if (rayIntersectsWithPolygon(ray, &node->polygons[i], &info->closestIntersection, &normal, &uv)) {
					hasHit = true;
					info->type = hitTypePolygon;
					info->normal = normal;
					info->uv = uv;
					info->objIndex = node->polygons[i].polyIndex;
					info->mtlIndex = node->polygons[i].materialIndex;
				}
			}
			//TODO: Clean this up
			if (hasHit) {
				info->hasHit = true;
				return true;
			}
			return false;
		}
	}
	return false;
}

bool rayIntersectsWithSphereTemp(struct sphere *sphere, struct lightRay *ray, struct shadeInfo *info) {
	if (rayIntersectsWithSphere(ray, sphere, &info->closestIntersection)) {
		info->type = hitTypeSphere;
		//Compute normal
		struct vector scaled = vectorScale(info->closestIntersection, &ray->direction);
		struct vector hitpoint = addVectors(&ray->start, &scaled);
		struct vector surfaceNormal = subtractVectors(&hitpoint, &sphere->pos);
		double temp = scalarProduct(&surfaceNormal,&surfaceNormal);
		if (temp == 0.0f) return false; //FIXME: Check this later
		temp = invsqrtf(temp);
		info->normal = vectorScale(temp, &surfaceNormal);
		return true;
	} else {
		info->type = hitTypeNone;
		return false;
	}
}

#define SMOOTH
//#define UV

void getSurfaceProperties(int polyIndex,
					  const struct coord uv,
					  struct vector *calculatedNormal,
					  struct coord *textureCoord) {
#ifdef SMOOTH
	//If smooth shading enabled
	//FIXME: Temporary hack to fix mainScene.obj lacking normals
	if (polygonArray[polyIndex].hasNormals) {
		struct vector n0 = normalArray[polygonArray[polyIndex].normalIndex[0]];
		struct vector n1 = normalArray[polygonArray[polyIndex].normalIndex[1]];
		struct vector n2 = normalArray[polygonArray[polyIndex].normalIndex[2]];
		
		// (1 - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2;
		
		struct vector scaled0 = vectorScale((1 - uv.x - uv.y), &n0);
		struct vector scaled1 = vectorScale(uv.x, &n1);
		struct vector scaled2 = vectorScale(uv.y, &n2);
		
		struct vector add0 = addVectors(&scaled0, &scaled1);
		struct vector add1 = addVectors(&add0, &scaled2);
		
		*calculatedNormal = add1;
		
		/*struct vector add0 = addVectors(&scaled0, &scaled1);
		struct vector add1 = vectorScale(uv.y, &n2);
		
		*calculatedNormal = addVectors(&add0, &add1);*/
	}
#endif

	*calculatedNormal = normalizeVector(calculatedNormal);
	
#ifdef UV
	//Texture coords
	struct vector s0 = textureArray[polygonArray[polyIndex].textureIndex[0]];
	struct vector s1 = textureArray[polygonArray[polyIndex].textureIndex[1]];
	struct vector s2 = textureArray[polygonArray[polyIndex].textureIndex[2]];
	
	// (1 - uv.x - uv.y) * st0 + uv.x * st1 + uv.y * st2;
	
	double u = (1 - uv.x - uv.y);
	u = u * s0.x;
	u = u * s0.y;
	u = u * s0.z;
	
	//double v = vectorScale(uv.x, &s1), vectorScale(uv.y, s2);
	
	//textureCoord = uvFromValues(u, v);
#endif
}

struct intersection getClosestIsect(struct lightRay *incidentRay, struct scene *worldScene) {
	struct intersection isect;
	struct shadeInfo info;
	
	isect.ray = incidentRay;
	isect.start = &incidentRay->currentMedium;
	isect.didIntersect = false;
	isect.distance = 20000.0f;
	int objCount = worldScene->objCount;
	int sphereCount = worldScene->sphereCount;
	
	for (int i = 0; i < sphereCount; i++) {
		if (rayIntersectsWithSphereTemp(&worldScene->spheres[i], incidentRay, &info)) {
			isect.end = &worldScene->materials[worldScene->spheres[i].materialIndex];
			isect.surfaceNormal = &info.normal;
			isect.didIntersect = true;
			isect.distance = info.closestIntersection;
			
			struct vector scaled = vectorScale(info.closestIntersection, &incidentRay->direction);
			struct vector hitPoint = addVectors(&incidentRay->start, &scaled);
			isect.hitPoint = &hitPoint;
		}
	}
	
	//Note: rayIntersectsWithNode makes sure this isect is closer than a possible sphere
	//intersect that happened in the previous check^.
	for (int o = 0; o < objCount; o++) {
		if (rayIntersectsWithNode(worldScene->objs[o].tree, incidentRay, &info)) {
			isect.end = &worldScene->objs[o].materials[info.mtlIndex];
			isect.surfaceNormal = &info.normal;
			isect.didIntersect = true;
			isect.distance = info.closestIntersection;
			
			struct vector scaled = vectorScale(info.closestIntersection, &incidentRay->direction);
			struct vector hitPoint = addVectors(&incidentRay->start, &scaled);
			isect.hitPoint = &hitPoint;
		}
	}
	
	return isect;
}

struct color getAmbient(struct intersection *isect, struct color *color) {
	return colorCoef(0.25, color);
}

bool isInShadow(struct lightRay *ray, double distance, struct scene *world) {
	
	struct intersection isect = getClosestIsect(ray, world);
	
	return isect.didIntersect && isect.distance < distance;
}

struct color getSpecular(struct intersection *isect, struct light *light) {
	struct color specular = (struct color){0.0, 0.0, 0.0, 0.0};
	double gloss = isect->end->glossiness;
	
	if (gloss == 0.0f) {
		//Not glossy, abort early
		return specular;
	}
	
	
	
	return specular;
}

struct color getHighlights(struct intersection *isect, struct color *color, struct scene *world) {
	//diffuse and specular highlights
	struct color diffuse = (struct color){0.0, 0.0, 0.0, 0.0};
	struct color specular = (struct color){0.0, 0.0, 0.0, 0.0};
	
	for (int i = 0; i < world->lightCount; i++) {
		struct light currentLight = world->lights[i];
		struct vector lightPos;
		lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
		struct vector lightDir = subtractVectors(&lightPos, isect->hitPoint);
		double distanceToLight = vectorLength(&lightDir);
		double dotProduct = scalarProduct(isect->surfaceNormal, &lightDir);
		
		if (dotProduct >= 0.0f) {
			//Intersection point is facing this light
			//Check if there are objects in the way to get shadows
			
			struct lightRay shadowRay;
			shadowRay.rayType = rayTypeShadow;
			shadowRay.start = *isect->hitPoint;
			shadowRay.direction = lightDir;
			shadowRay.currentMedium = isect->ray->currentMedium;
			
			if (isInShadow(&shadowRay, distanceToLight, world)) {
				//Something is in the way, stop here and test other lights
				continue;
			}
			
			struct color temp = colorCoef(dotProduct, color);
			struct color tempDiff = addColors(&diffuse, &temp);
			diffuse = multiplyColors(&currentLight.intensity, &tempDiff);
			
			struct color specTemp = getSpecular(isect, &currentLight);
			specular = addColors(&specular, &specTemp);
		}
		
	}
	
	return addColors(&diffuse, &specular);
}

struct color getReflectsAndRefracts(struct intersection *isect, struct color *color) {
	//Interacted light, so refracted and reflected rays
	
	
	return (struct color){};
}

struct color getLighting(struct intersection *isect, struct scene *world) {
	struct color output = output = isect->end->diffuse;
	
	struct color ambientColor = getAmbient(isect, &output);
	struct color highlights = getHighlights(isect, &output, world);
	struct color interacted = getReflectsAndRefracts(isect, &output);
	
	struct color temp = addColors(&ambientColor, &highlights);
	return addColors(&temp, &interacted);
}

struct color newTrace(struct lightRay *incidentRay, struct scene *worldScene) {
	
	struct intersection closestIsect = getClosestIsect(incidentRay, worldScene);
	
	if (closestIsect.didIntersect) {
		return getLighting(&closestIsect, worldScene);
	} else {
		return *worldScene->ambientColor;
	}
	
}

/**
 Returns a computed color based on a given ray and world scene
 
 @param incidentRay View ray to be cast into a scene
 @param worldScene Scene the ray is cast into
 @return Color value with full precision (double)
 */
struct color rayTrace(struct lightRay *incidentRay, struct scene *worldScene) {
	//Raytrace a given light ray with a given scene, then return the color value for that ray
	struct color output = {0.0f,0.0f,0.0f};
	int bounces = 0;
	double contrast = worldScene->camera->contrast;
	
	struct shadeInfo *isectInfo = (struct shadeInfo*)calloc(1, sizeof(struct shadeInfo));
	struct shadeInfo *shadowInfo = (struct shadeInfo*)calloc(1, sizeof(struct shadeInfo));
	
	do {
		//closestIntersection, also often called 't', distance to closest intersection
		//Used to figure out the nearest intersection
		double closestIntersection = 20000.0f;
		double temp;
		int currentSphere = -1;
		int currentPolygon = -1;
		int sphereAmount = worldScene->sphereCount;
		int lightSourceAmount = worldScene->lightCount;
		int objCount = worldScene->objCount;
		
		struct material currentMaterial;
		struct vector surfaceNormal = {0.0, 0.0, 0.0};
		struct coord  uv          = {0.0, 0.0};
		struct coord textureCoord = {0.0, 0.0};
		struct vector hitpoint;
		
		unsigned int i;
		for (i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &closestIntersection)) {
				currentSphere = i;
				currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].materialIndex];
			}
		}
		
		isectInfo->closestIntersection = closestIntersection;
		isectInfo->normal = surfaceNormal;
		unsigned int o;
		for (o = 0; o < objCount; o++) {
			if (rayIntersectsWithNode(worldScene->objs[o].tree, incidentRay, isectInfo)) {
				currentPolygon      = isectInfo->objIndex;
				closestIntersection = isectInfo->closestIntersection;
				surfaceNormal          = isectInfo->normal;
				uv                  = isectInfo->uv;
				getSurfaceProperties(isectInfo->objIndex, uv, &surfaceNormal, &textureCoord);
				currentMaterial = worldScene->objs[o].materials[isectInfo->mtlIndex];
				currentSphere = -1;
			}
		}
		
		//Ray-object intersection detection
		if (currentSphere != -1) {
			struct vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			surfaceNormal = subtractVectors(&hitpoint, &worldScene->spheres[currentSphere].pos);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
		} else if (currentPolygon != -1) {
			struct vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0f) break;
			temp = invsqrtf(temp);
			//FIXME: Possibly get existing normal here
			surfaceNormal = vectorScale(temp, &surfaceNormal);
		} else {
			//Ray didn't hit any object, set color to ambient
			struct color temp = colorCoef(contrast, worldScene->ambientColor);
			output = addColors(&output, &temp);
			break;
		}
		
		if (scalarProduct(&surfaceNormal, &incidentRay->direction) < 0.0f) {
			surfaceNormal = vectorScale(1.0f, &surfaceNormal);
		} else if (scalarProduct(&surfaceNormal, &incidentRay->direction) > 0.0f) {
			surfaceNormal = vectorScale(-1.0f, &surfaceNormal);
		}
		
		struct lightRay bouncedRay;
		bouncedRay.start = hitpoint;
		
		//Find the value of the light at this point
		unsigned int j;
		for (j = 0; j < lightSourceAmount; ++j) {
			struct light currentLight = worldScene->lights[j];
			struct vector lightPos;
			if (worldScene->camera->areaLights)
				lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
			else
				lightPos = currentLight.pos;
			
			bouncedRay.direction = subtractVectors(&lightPos, &hitpoint);
			
			double lightProjection = scalarProduct(&bouncedRay.direction, &surfaceNormal);
			if (lightProjection <= 0.0f) continue;
			
			double lightDistance = scalarProduct(&bouncedRay.direction, &bouncedRay.direction);
			double temp = lightDistance;
			
			if (temp <= 0.0f) continue;
			temp = invsqrtf(temp);
			bouncedRay.direction = vectorScale(temp, &bouncedRay.direction);
			lightProjection = temp * lightProjection;
			
			//Calculate shadows
			bool inShadow = false;
			double t = lightDistance;
			unsigned int k;
			for (k = 0; k < sphereAmount; ++k) {
				if (rayIntersectsWithSphere(&bouncedRay, &worldScene->spheres[k], &t)) {
					inShadow = true;
					break;
				}
			}
			
			shadowInfo->closestIntersection = t;
			shadowInfo->normal = surfaceNormal;
			for (o = 0; o < objCount; o++) {
				if (rayIntersectsWithNode(worldScene->objs[o].tree, &bouncedRay, shadowInfo)) {
					t = shadowInfo->closestIntersection;
					inShadow = true;
					break;
				}
			}
			
			if (!inShadow) {
				//TODO: Calculate specular reflection
				float specularFactor = 1.0;//scalarProduct(&cameraRay.direction, &surfaceNormal) * contrast;
				
				//Calculate Lambert diffusion
				float diffuseFactor = scalarProduct(&bouncedRay.direction, &surfaceNormal) * contrast;
				output.red += specularFactor * diffuseFactor * currentLight.intensity.red * currentMaterial.diffuse.red;
				output.green += specularFactor * diffuseFactor * currentLight.intensity.green * currentMaterial.diffuse.green;
				output.blue += specularFactor * diffuseFactor * currentLight.intensity.blue * currentMaterial.diffuse.blue;
			}
		}
		//Iterate over the reflection
		contrast *= currentMaterial.reflectivity;
		
		//Calculate reflected ray start and direction
		double reflect = 2.0f * scalarProduct(&incidentRay->direction, &surfaceNormal);
		incidentRay->start = hitpoint;
		struct vector tempVec = vectorScale(reflect, &surfaceNormal);
		incidentRay->direction = subtractVectors(&incidentRay->direction, &tempVec);
		
		bounces++;
		
	} while ((contrast > 0.0f) && (bounces <= worldScene->camera->bounces));
	
	free(isectInfo);
	free(shadowInfo);
	
	return output;
}
