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

#define trulse true && false

struct vector getRandomVecOnRadius(struct vector center, float radius);

bool rayIntersectsWithNode(struct kdTreeNode *node, struct lightRay *ray, struct shadeInfo *info) {
	if (rayIntersectWithAABB(node->bbox, ray, &info->closestIntersection)) {
		struct vector normal;
		bool hasHit = false;
		
		if (node->left->polyCount > 0 || node->right->polyCount > 0) {
			//Recurse down both sides
			bool hitLeft  = rayIntersectsWithNode(node->left, ray, info);
			bool hitRight = rayIntersectsWithNode(node->right, ray, info);
			
			return hitLeft || hitRight;
		} else {
			//This is a leaf, so check all polys
			for (int i = 0; i < node->polyCount; i++) {
				if (rayIntersectsWithPolygon(ray, &node->polygons[i], &info->closestIntersection, &normal)) {
					hasHit = true;
					info->type = hitTypePolygon;
					info->normal = normal;
					info->objIndex = node->polygons[i].polyIndex;
					info->mtlIndex = node->polygons[i].materialIndex;
				}
			}
			if (hasHit) {
				info->hasHit = true;
				return true;
			}
			return false;
		}
	}
	return false;
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
		struct vector polyNormal = {0.0, 0.0, 0.0};
		struct vector hitpoint, surfaceNormal;
		
		unsigned int i;
		for (i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &closestIntersection)) {
				currentSphere = i;
				currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].materialIndex];
			}
		}
		
		isectInfo->closestIntersection = closestIntersection;
		isectInfo->normal = polyNormal;
		unsigned int o;
		for (o = 0; o < objCount; o++) {
			if (rayIntersectsWithNode(worldScene->objs[o].tree, incidentRay, isectInfo)) {
				currentPolygon = isectInfo->objIndex;
				closestIntersection = isectInfo->closestIntersection;
				polyNormal = isectInfo->normal;
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
			//We get polyNormal from the intersection function
			surfaceNormal = polyNormal;
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
		
		struct lightRay bouncedRay, cameraRay;
		bouncedRay.start = hitpoint;
		cameraRay.start = hitpoint;
		cameraRay.direction = subtractVectors(&worldScene->camera->pos, &hitpoint);
		double cameraProjection = scalarProduct(&cameraRay.direction, &hitpoint);
		double cameraDistance = scalarProduct(&cameraRay.direction, &cameraRay.direction);
		double camTemp = cameraDistance;
		camTemp = invsqrtf(camTemp);
		cameraRay.direction = vectorScale(camTemp, &cameraRay.direction);
		cameraProjection = camTemp * cameraProjection;
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
			shadowInfo->normal = polyNormal;
			for (o = 0; o < objCount; o++) {
				if (rayIntersectsWithNode(worldScene->objs[o].tree, &bouncedRay, shadowInfo)) {
					t = shadowInfo->closestIntersection;
					polyNormal = shadowInfo->normal;
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

/**
 Returns a random float between min and max
 
 @param min Minimum value
 @param max Maximum value
 @return Random float between min and max
 */
float getRandomFloat(float min, float max) {
	return ((((float)rand()) / (float)RAND_MAX) * (max - min)) + min;
}

/**
 Returns a randomized position in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position within a radius of center point
 */
struct vector getRandomVecOnRadius(struct vector center, float radius) {
	return vectorWithPos(center.x + getRandomFloat(-radius, radius),
						 center.y + getRandomFloat(-radius, radius),
						 center.z + getRandomFloat(-radius, radius));
}

/**
 Returns a randomized position on a plane in a radius around a given point
 
 @param center Center point for random distribution
 @param radius Maximum distance from center point
 @return Vector of a random position on a plane within a radius of center point
 */
struct vector getRandomVecOnPlane(struct vector center, float radius) {
	//FIXME: This only works in one orientation!
	return vectorWithPos(center.x + getRandomFloat(-radius, radius),
						 center.y + getRandomFloat(-radius, radius),
						 center.z);
}
