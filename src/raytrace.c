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

/**
 Traverse a k-d tree and see if a ray collides with a polygon.

 @param node Given tree to traverse
 @param ray Ray to check intersection on
 @param info Shading information
 @return True if ray hits a polygon in a leaf node, otherwise false
 */
bool rayIntersectsWithNode(struct kdTreeNode *node, struct lightRay *ray, struct intersection *isect) {
	//A bit of a hack, but it does work...!
	double fakeIsect = 20000.0;
	if (rayIntersectWithAABB(node->bbox, ray, &fakeIsect)) {
		bool hasHit = false;
		
		if (node->left->polyCount > 0 || node->right->polyCount > 0) {
			//Recurse down both sides
			bool hitLeft  = rayIntersectsWithNode(node->left, ray, isect);
			bool hitRight = rayIntersectsWithNode(node->right, ray, isect);
			
			return hitLeft || hitRight;
		} else {
			//This is a leaf, so check all polys
			for (int i = 0; i < node->polyCount; i++) {
				if (rayIntersectsWithPolygon(ray, &node->polygons[i], &isect->distance, &isect->surfaceNormal, &isect->uv)) {
					hasHit = true;
					isect->type = hitTypePolygon;
					isect->polyIndex = node->polygons[i].polyIndex;
					isect->mtlIndex = node->polygons[i].materialIndex;
					struct vector scaled = vectorScale(isect->distance, &ray->direction);
					isect->hitPoint = addVectors(&ray->start, &scaled);
				}
			}
			if (hasHit) {
				isect->didIntersect = true;
				return true;
			} else {
				return false;
			}
		}
	}
	return false;
}

bool rayIntersectsWithSphereTemp(struct sphere *sphere, struct lightRay *ray, struct intersection *isect) {
	//Pass the distance value to rayIntersectsWithSphere, where it's set
	if (rayIntersectsWithSphere(ray, sphere, &isect->distance)) {
		isect->type = hitTypeSphere;
		//Compute normal and store it to isect
		struct vector scaled = vectorScale(isect->distance, &ray->direction);
		struct vector hitpoint = addVectors(&ray->start, &scaled);
		struct vector surfaceNormal = subtractVectors(&hitpoint, &sphere->pos);
		double temp = scalarProduct(&surfaceNormal,&surfaceNormal);
		if (temp == 0.0) return false; //FIXME: Check this later
		temp = invsqrt(temp);
		isect->surfaceNormal = vectorScale(temp, &surfaceNormal);
		//Also store hitpoint
		isect->hitPoint = hitpoint;
		return true;
	} else {
		isect->type = hitTypeNone;
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


/**
 Calculate the closest intersection point, and other relevant information based on a given lightRay and worldScene
 See the intersection struct for documentation of what this function calculates.

 @param incidentRay Given light ray (set up in renderThread())
 @param worldScene  Given worldScene to cast that ray into
 @return intersection struct with the appropriate values set
 */
struct intersection getClosestIsect(struct lightRay *incidentRay, struct scene *worldScene) {
	struct intersection isect;
	memset(&isect, 0, sizeof(isect));
	
	//Initial distance. This could be some 'max' value, but 20k is enough in most cases.
	//This is used to keep track of the 'closest' intersection point
	isect.distance = 20000.0;
	isect.ray = *incidentRay;
	isect.start = incidentRay->currentMedium;
	isect.didIntersect = false;
	int objCount = worldScene->objCount;
	int sphereCount = worldScene->sphereCount;
	
	//First check all spheres to see if this ray intersects with them
	//We pass isect to rayIntersectsWithSphereTemp, which sets stuff like hitPoint, normal and distance
	for (int i = 0; i < sphereCount; i++) {
		if (rayIntersectsWithSphereTemp(&worldScene->spheres[i], incidentRay, &isect)) {
			isect.end = worldScene->materials[worldScene->spheres[i].materialIndex];
			isect.didIntersect = true;
		}
	}
	
	//Note: rayIntersectsWithNode makes sure this isect is closer than a possible sphere
	//So if it finds an intersection that is farther away than the intersection we found above with
	//a sphere, it will return false
	//This is how most raytracers solve the visibility problem.
	//intersect that happened in the previous check^.
	for (int o = 0; o < objCount; o++) {
		if (rayIntersectsWithNode(worldScene->objs[o].tree, incidentRay, &isect)) {
			isect.end = worldScene->objs[o].materials[polygonArray[isect.polyIndex].materialIndex];
			isect.didIntersect = true;
		}
	}
	
	return isect;
}

struct color getAmbient(const struct intersection *isect, struct color *color) {
	//Very cheap ambient occlusion
	return colorCoef(0.25, color);
}


/**
 Check if this spot is in shadow

 @param ray Bounced light ray (oriented towards light)
 @param distance Distance to light, if a possible intersection is closer than this, the spot is in shadow
 @param world World scene for scene data
 @return Boolean if spot is in shadow or not.
 */
bool isInShadow(struct lightRay *ray, double distance, struct scene *world) {
	
	struct intersection isect = getClosestIsect(ray, world);
	
	return isect.didIntersect && isect.distance < distance;
}


/**
 Compute reflection vector from a given vector and surface normal

 @param vec Incident ray to reflect
 @param normal Surface normal at point of reflection
 @return Reflected vector
 */
struct vector reflectVec(const struct vector *incident, const struct vector *normal) {
	double reflect = 2.0 * scalarProduct(incident, normal);
	struct vector temp = vectorScale(reflect, normal);
	return subtractVectors(incident, &temp);
}


/**
 Compute refraction vector from a given vector and surface normal

 @param incident Given light ray
 @param normal Surface normal at point of intersection
 @param startIOR Starting material index of refraction
 @param endIOR Intersected object index of refraction
 @return Refracted vector
 */
struct vector refractVec(const struct vector *incident, const struct vector *normal, const double startIOR, const double endIOR) {
	double ratio = startIOR / endIOR;
	double cosI = -scalarProduct(normal, incident);
	double sinT2 = ratio * ratio * (1.0 - cosI * cosI);
	
	if (sinT2 > 1.0) {
		printf("Bad refraction encountered.\n");
		exit(-19);
	}
	
	double cosT = sqrt(1.0 - sinT2);
	struct vector temp1 = vectorScale(ratio, incident);
	struct vector temp2 = vectorScale((ratio * cosI - cosT), normal);
	
	return addVectors(&temp1, &temp2);
	
}


/**
 Compute specular highlights for an intersection point

 @param isect Intersection point
 @param light Given light that casts a highlight
 @param lightPos The randomized distribution (temporary) light position for given light.
 @return Specular highlight color
 */
struct color getSpecular(const struct intersection *isect, struct light *light, struct vector *lightPos) {
	struct color specular = (struct color){0.0, 0.0, 0.0, 0.0};
	double gloss = isect->end.glossiness;
	
	if (gloss == 0.0) {
		//Not glossy, abort early
		return specular;
	}
	
	struct vector viewTemp = subtractVectors(&isect->ray.start, &isect->hitPoint);
	struct vector view = normalizeVector(&viewTemp);
	
	struct vector lightDir = subtractVectors(lightPos, &isect->hitPoint);
	struct vector lightDirNormal = normalizeVector(&lightDir);
	struct vector reflected = reflectVec(&lightDirNormal, &isect->surfaceNormal);
	
	double dotProduct = scalarProduct(&view, &reflected);
	if (dotProduct <= 0) {
		return specular;
	}
	
	specular.red   = pow(dotProduct, gloss) * light->intensity.red;
	specular.green = pow(dotProduct, gloss) * light->intensity.green;
	specular.blue  = pow(dotProduct, gloss) * light->intensity.blue;
	
	return specular;
}


/**
 Calculate the diffuse and specular components of lighting, as well as shadows

 @param isect Intersection information
 @param color Base color of the object
 @param world Scene to get lighting information
 @return Highlighted color
 */
struct color getHighlights(const struct intersection *isect, struct color *color, struct scene *world) {
	//diffuse and specular highlights
	struct color  diffuse = (struct color){0.0, 0.0, 0.0, 0.0};
	struct color specular = (struct color){0.0, 0.0, 0.0, 0.0};
	
	for (int i = 0; i < world->lightCount; i++) {
		struct light currentLight = world->lights[i];
		struct vector lightPos = vectorWithPos(0, 0, 0);
		lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
		
		struct vector lightOffset = subtractVectors(&lightPos, &isect->hitPoint);
		double distance = vectorLength(&lightOffset);
		struct vector lightDir = normalizeVector(&lightOffset);
		double dotProduct = scalarProduct(&isect->surfaceNormal, &lightDir);
		
		if (dotProduct >= 0.0) {
			//Intersection point is facing this light
			//Check if there are objects in the way to get shadows
			
			struct lightRay shadowRay;
			shadowRay.rayType = rayTypeShadow;
			shadowRay.start = isect->hitPoint;
			shadowRay.direction = lightDir;
			shadowRay.currentMedium = isect->ray.currentMedium;
			shadowRay.remainingInteractions = 1;
			
			if (isInShadow(&shadowRay, distance, world)) {
				//Something is in the way, stop here and test other lights
				continue;
			}
			
			struct color coef = colorCoef(dotProduct, color);
			struct color diffuseCoef = addColors(&diffuse, &coef);
			diffuse = multiplyColors(&diffuseCoef, &currentLight.intensity);
			
			struct color specTemp = getSpecular(isect, &currentLight, &lightPos);
			specular = addColors(&specular, &specTemp);
		}
		
	}
	
	return addColors(&diffuse, &specular);
}


/**
 Calculate ratio of reflection/refraction based on the angle of intersection and IORs
 Imagine light reflecting off a surface of a glass object vs the light showing through
 at that point.

 @param normal Surface normal
 @param dir Direction of intersected vector (light ray hitting object)
 @param startIOR Index of refraction of the material the ray was cast from
 @param endIOR   Index of refraction of the material the ray intersected with
 @return Reflectance "percentage", from which the refractance amount can be calculated
 */
double getReflectance(const struct vector *normal, const struct vector *dir, double startIOR, double endIOR) {
	double ratio = startIOR / endIOR;
	double cosI = -scalarProduct(normal, dir);
	double sinT2 = ratio * ratio * (1.0 - cosI * cosI);
	
	if (sinT2 > 1.0) {
		return 1.0;
	}
	
	double cosT = sqrt(1.0 - sinT2);
	double r0rth = (startIOR * cosI - endIOR * cosT) / (startIOR * cosI + endIOR * cosT);
	double rPar = (endIOR * cosI - startIOR * cosT) / (endIOR * cosI + startIOR * cosT);
	
	return (r0rth * r0rth + rPar * rPar) / 2.0;
}


/**
 Compute reflected and refracted effects

 @param isect Intersection point
 @param color Base color
 @param world World scene for recursion
 @return Reflect/refract color
 */
struct color getReflectsAndRefracts(const struct intersection *isect, struct color *color, struct scene *world) {
	//Interacted light, so refracted and reflected rays
	double reflectivity = isect->end.reflectivity;
	double startIOR     = isect->start.IOR;
	double   endIOR     = isect->end  .IOR;
	int remainingInteractions = isect->ray.remainingInteractions;
	
	if ((reflectivity == NOT_REFLECTIVE && endIOR == NOT_REFRACTIVE) || remainingInteractions <= 0) {
		return (struct color){0.0, 0.0, 0.0, 0.0};
	}
	
	double reflectivePercentage = reflectivity;
	double refractivePercentage = 0;
	
	//Get ratio of reflection/refraction
	if (isect->end.IOR != NOT_REFRACTIVE) {
		//FIXME: getReflectance returns +inf
		reflectivePercentage = getReflectance(&isect->surfaceNormal, &isect->ray.direction, isect->start.IOR, isect->end.IOR);
		refractivePercentage = 1 - reflectivePercentage;
	}
	
	//End early if no interactive properties
	if (refractivePercentage <= 0 && reflectivePercentage <= 0) {
		return (struct color){0.0, 0.0, 0.0, 0.0};
	}
	
	struct color reflectiveColor = (struct color){0.0, 0.0, 0.0, 0.0};
	struct color refractiveColor = (struct color){0.0, 0.0, 0.0, 0.0};
	
	//Recursively trace new rays to reflect and refract
	
	if (reflectivePercentage > 0) {
		struct vector reflected = reflectVec(&isect->ray.direction, &isect->surfaceNormal);
		struct lightRay reflectedRay;
		reflectedRay.start = isect->hitPoint;
		reflectedRay.direction = reflected;
		reflectedRay.remainingInteractions = remainingInteractions - 1;
		reflectedRay.currentMedium = isect->ray.currentMedium;
		//And recurse!
		struct color temp = newTrace(&reflectedRay, world);
		reflectiveColor = colorCoef(reflectivePercentage, &temp);
	}
	
	if (refractivePercentage > 0) {
		struct vector refracted = refractVec(&isect->ray.direction, &isect->surfaceNormal, startIOR, endIOR);
		struct lightRay refractedRay;
		refractedRay.start = isect->hitPoint;
		refractedRay.direction = refracted;
		refractedRay.remainingInteractions = remainingInteractions - 1;
		refractedRay.currentMedium = isect->end;
		//Recurse here too
		struct color temp = newTrace(&refractedRay, world);
		refractiveColor = colorCoef(refractivePercentage, &temp);
	}
	
	return addColors(&reflectiveColor, &refractiveColor);
}


/**
 Get the lighting for a given intersection point.
 getReflectsAndRefracts handles the recursive sending of reflection and refraction rays

 @param isect Intersection struct that stores information used to calculate shading
 @param world worldScene to get additional material and lighting information
 @return (Hopefully) correct color based on the given information.
 */
struct color getLighting(const struct intersection *isect, struct scene *world) {
	//Grab the 'base' diffuse color of the intersected object, and pass that to the
	//additional functions to add shading to it.
	struct color output = isect->end.diffuse;
	
	//getAmbient is a simple 'ambient occlusion' hack, works fine.
	struct color ambientColor = getAmbient(isect, &output);
	//getHighlights doesn't seem to work at all. Supposed to produce surface shading (shadows and whatnot)
	struct color highlights = getHighlights(isect, &output, world);
	//Reflections seem to work okay, but refractions need to be fixed
	//Sphere reflections get a weird white band around the edges on optimized builds.
	struct color interacted = getReflectsAndRefracts(isect, &output, world);
	
	//Just add these colors together to get the final result
	struct color temp = addColors(&ambientColor, &highlights);
	return addColors(&temp, &interacted);
}


/**
 New, recursive raytracer. (Unfinished)
 Should support refractions, and will be easier to expand in the future.

 @param incidentRay Given light ray to cast into a scene
 @param worldScene Given scene to cast ray into
 @return Color based on the given ray and scene.
 */
struct color newTrace(struct lightRay *incidentRay, struct scene *worldScene) {
	//This is the start of the new rayTracer.
	//Start by getting the closest intersection point in worldScene for this given incidentRay.
	struct intersection closestIsect = getClosestIsect(incidentRay, worldScene);
	
	//Check if it hit something
	if (closestIsect.didIntersect) {
		//Ray has hit an object or a sphere, calculate the lighting
		return getLighting(&closestIsect, worldScene);
	} else {
		//Ray didn't hit anything, just return the ambientColor of this scene (set in scene.c)
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
	struct color output = {0.0,0.0,0.0,0.0};
	int bounces = 0;
	double contrast = worldScene->camera->contrast;
	
	struct intersection *isectInfo = (struct intersection*)calloc(1, sizeof(struct intersection));
	struct intersection *shadowInfo = (struct intersection*)calloc(1, sizeof(struct intersection));
	
	do {
		//closestIntersection, also often called 't', distance to closest intersection
		//Used to figure out the nearest intersection
		double closestIntersection = 20000.0;
		double temp;
		int currentSphere = -1;
		int currentPolygon = -1;
		unsigned sphereAmount = worldScene->sphereCount;
		unsigned lightSourceAmount = worldScene->lightCount;
		unsigned objCount = worldScene->objCount;
		
		struct material currentMaterial;
		struct vector surfaceNormal = {0.0, 0.0, 0.0, false};
		struct coord  uv          = {0.0, 0.0};
		struct coord textureCoord = {0.0, 0.0};
		struct vector hitpoint;
		
		for (unsigned i = 0; i < sphereAmount; ++i) {
			if (rayIntersectsWithSphere(incidentRay, &worldScene->spheres[i], &closestIntersection)) {
				currentSphere = i;
				currentMaterial = worldScene->materials[worldScene->spheres[currentSphere].materialIndex];
			}
		}
		
		isectInfo->distance = closestIntersection;
		isectInfo->surfaceNormal = surfaceNormal;
		for (unsigned o = 0; o < objCount; o++) {
			if (rayIntersectsWithNode(worldScene->objs[o].tree, incidentRay, isectInfo)) {
				currentPolygon      = isectInfo->polyIndex;
				closestIntersection = isectInfo->distance;
				surfaceNormal          = isectInfo->surfaceNormal;
				uv                  = isectInfo->uv;
				getSurfaceProperties(isectInfo->polyIndex, uv, &surfaceNormal, &textureCoord);
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
			if (temp == 0.0) break;
			temp = invsqrt(temp);
			surfaceNormal = vectorScale(temp, &surfaceNormal);
		} else if (currentPolygon != -1) {
			struct vector scaled = vectorScale(closestIntersection, &incidentRay->direction);
			hitpoint = addVectors(&incidentRay->start, &scaled);
			temp = scalarProduct(&surfaceNormal,&surfaceNormal);
			if (temp == 0.0) break;
			temp = invsqrt(temp);
			//FIXME: Possibly get existing normal here
			surfaceNormal = vectorScale(temp, &surfaceNormal);
		} else {
			//Ray didn't hit any object, set color to ambient
			struct color temp = colorCoef(contrast, worldScene->ambientColor);
			output = addColors(&output, &temp);
			break;
		}
		
		if (scalarProduct(&surfaceNormal, &incidentRay->direction) < 0.0) {
			surfaceNormal = vectorScale(1.0, &surfaceNormal);
		} else if (scalarProduct(&surfaceNormal, &incidentRay->direction) > 0.0) {
			surfaceNormal = vectorScale(-1.0, &surfaceNormal);
		}
		
		struct lightRay bouncedRay;
		bouncedRay.start = hitpoint;
		
		//Find the value of the light at this point
		for (unsigned j = 0; j < lightSourceAmount; ++j) {
			struct light currentLight = worldScene->lights[j];
			struct vector lightPos;
			if (worldScene->camera->areaLights)
				lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
			else
				lightPos = currentLight.pos;
			
			bouncedRay.direction = subtractVectors(&lightPos, &hitpoint);
			
			double lightProjection = scalarProduct(&bouncedRay.direction, &surfaceNormal);
			if (lightProjection <= 0.0) continue;
			
			double lightDistance = scalarProduct(&bouncedRay.direction, &bouncedRay.direction);
			double temp = lightDistance;
			
			if (temp <= 0.0) continue;
			temp = invsqrt(temp);
			bouncedRay.direction = vectorScale(temp, &bouncedRay.direction);
			
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
			
			shadowInfo->distance = t;
			shadowInfo->surfaceNormal = surfaceNormal;
			for (unsigned o = 0; o < objCount; o++) {
				if (rayIntersectsWithNode(worldScene->objs[o].tree, &bouncedRay, shadowInfo)) {
					t = shadowInfo->distance;
					inShadow = true;
					break;
				}
			}
			
			if (!inShadow) {
				//TODO: Calculate specular reflection
				double specularFactor = 1.0;//scalarProduct(&cameraRay.direction, &surfaceNormal) * contrast;
				
				//Calculate Lambert diffusion
				double diffuseFactor = scalarProduct(&bouncedRay.direction, &surfaceNormal) * contrast;
				output.red += specularFactor * diffuseFactor * currentLight.intensity.red * currentMaterial.diffuse.red;
				output.green += specularFactor * diffuseFactor * currentLight.intensity.green * currentMaterial.diffuse.green;
				output.blue += specularFactor * diffuseFactor * currentLight.intensity.blue * currentMaterial.diffuse.blue;
			}
		}
		//Iterate over the reflection
		contrast *= currentMaterial.reflectivity;
		
		//Calculate reflected ray start and direction
		double reflect = 2.0 * scalarProduct(&incidentRay->direction, &surfaceNormal);
		incidentRay->start = hitpoint;
		struct vector tempVec = vectorScale(reflect, &surfaceNormal);
		incidentRay->direction = subtractVectors(&incidentRay->direction, &tempVec);
		
		bounces++;
		
	} while ((contrast > 0.0) && (bounces <= worldScene->camera->bounces));
	
	free(isectInfo);
	free(shadowInfo);
	
	return output;
}
