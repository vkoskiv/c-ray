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

//TODO: Merge this functionality into rayIntersectsWithSphere
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
 Calculate the closest intersection point, and other relevant information based on a given lightRay and scene
 See the intersection struct for documentation of what this function calculates.

 @param incidentRay Given light ray (set up in renderThread())
 @param scene  Given scene to cast that ray into
 @return intersection struct with the appropriate values set
 */
struct intersection getClosestIsect(struct lightRay *incidentRay, struct world *scene) {
	struct intersection isect;
	memset(&isect, 0, sizeof(isect));
	
	//Initial distance. This could be some 'max' value, but 20k is enough in most cases.
	//This is used to keep track of the 'closest' intersection point
	isect.distance = 20000.0;
	isect.ray = *incidentRay;
	isect.start = incidentRay->currentMedium;
	isect.didIntersect = false;
	int objCount = scene->objCount;
	int sphereCount = scene->sphereCount;
	
	//First check all spheres to see if this ray intersects with them
	//We pass isect to rayIntersectsWithSphereTemp, which sets stuff like hitPoint, normal and distance
	for (int i = 0; i < sphereCount; i++) {
		if (rayIntersectsWithSphereTemp(&scene->spheres[i], incidentRay, &isect)) {
			isect.end = scene->spheres[i].material;
			isect.didIntersect = true;
		}
	}
	
	//Note: rayIntersectsWithNode makes sure this isect is closer than a possible sphere
	//So if it finds an intersection that is farther away than the intersection we found above with
	//a sphere, it will return false
	//This is how most raytracers solve the visibility problem.
	//intersect that happened in the previous check^.
	for (int o = 0; o < objCount; o++) {
		if (rayIntersectsWithNode(scene->objs[o].tree, incidentRay, &isect)) {
			isect.end = scene->objs[o].materials[polygonArray[isect.polyIndex].materialIndex];
			isect.didIntersect = true;
		}
	}
	
	return isect;
}

struct color getAmbient(const struct intersection *isect, struct color *color) {
	//Very cheap global illumination
	return colorCoef(0.25, color);
}

/**
 Check if this spot is in shadow

 @param ray Bounced light ray (oriented towards light)
 @param distance Distance to light, if a possible intersection is closer than this, the spot is in shadow
 @param world World scene for scene data
 @return Boolean if spot is in shadow or not.
 */
bool isInShadow(struct lightRay *ray, double distance, struct world *scene) {
	
	struct intersection isect = getClosestIsect(ray, scene);
	
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
	
	specular.red   = pow(dotProduct, gloss) * light->diffuse.red;
	specular.green = pow(dotProduct, gloss) * light->diffuse.green;
	specular.blue  = pow(dotProduct, gloss) * light->diffuse.blue;
	
	return specular;
}

/**
 Calculate the diffuse and specular components of lighting, as well as shadows

 @param isect Intersection information
 @param color Base color of the object
 @param world Scene to get lighting information
 @return Highlighted color
 */
struct color getHighlights(struct intersection *isect, struct color *color, struct world *scene) {
	//diffuse and specular highlights
	struct color  diffuse = (struct color){0.0, 0.0, 0.0, 0.0};
	struct color specular = (struct color){0.0, 0.0, 0.0, 0.0};
	
	struct vector N = isect->surfaceNormal;

	for (int i = 0; i < scene->lightCount; ++i) {
		struct light currentLight = scene->lights[i];
		struct vector lightPos;
		
		if (scene->areaLights)
			lightPos = getRandomVecOnRadius(currentLight.pos, currentLight.radius);
		else
			lightPos = currentLight.pos;
		
		struct vector lightOffset = subtractVectors(&lightPos, &isect->hitPoint);
		double distance = vectorLength(&lightOffset);
		struct vector L = normalizeVector(&lightOffset);
		double NdotL = scalarProduct(&N, &L);

		if (NdotL >= 0.0) {
			//Intersection point is facing this light
			//Check if there are objects in the way to get shadows
			
			struct lightRay shadowRay;
			shadowRay.rayType = rayTypeShadow;
			shadowRay.start = isect->hitPoint;
			shadowRay.direction = L;
			shadowRay.currentMedium = isect->ray.currentMedium;
			shadowRay.remainingInteractions = 1;
			
			if (isInShadow(&shadowRay, distance, scene)) {
				//Something is in the way, stop here and test other lights
				continue;
			}

			double intensity = min(max(NdotL,0),1);
			struct color diffTmp = colorCoef(intensity*(currentLight.radius/distance), color);
			diffuse = addColors(&diffuse, &diffTmp);
		
			struct vector forward = vectorCross(&scene->camera->left, &scene->camera->up);
			struct vector H = addVectors(&L, &forward);
			H = normalizeVector(&H);

			double NdotH = scalarProduct(&N, &H);
			double specAngle = max(NdotH, 0.0);
			double specVal = pow(specAngle, isect->end.glossiness);

			struct color specTmp = colorCoef(specVal*(currentLight.radius/distance), &isect->end.specular);
			specular = addColors(&specular, &specTmp);
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
	
	return min(max((r0rth * r0rth + rPar * rPar) / 2.0,0.0), 1.0);
}

/**
 Compute reflected and refracted effects

 @param isect Intersection point
 @param color Base color
 @param scene scene scene for recursion
 @return Reflect/refract color
 */
struct color getReflectsAndRefracts(const struct intersection *isect, struct color *color, struct world *scene) {
	//Interacted light, so refracted and reflected rays
	double reflectivity = isect->end.reflectivity;
	double startIOR     = isect->start.IOR;
	double   endIOR     = isect->end  .IOR;
	//An interaction is either a reflection or refraction
	int remainingInteractions = isect->ray.remainingInteractions;
	
	//If it won't reflect or refract, don't bother calculating and stop here
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
		struct color temp = rayTrace(&reflectedRay, scene);
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
		struct color temp = rayTrace(&refractedRay, scene);
		refractiveColor = colorCoef(refractivePercentage, &temp);
	}
	
	return addColors(&reflectiveColor, &refractiveColor);
}

/**
 Get the lighting for a given intersection point.
 getReflectsAndRefracts handles the recursive sending of reflection and refraction rays

 @param isect Intersection struct that stores information used to calculate shading
 @param scene scene to get additional material and lighting information
 @return (Hopefully) correct color based on the given information.
 */
struct color getLighting(struct intersection *isect, struct world *scene) {
	struct coord textureCoord = {0.0, 0.0};
	if(isect->type == hitTypePolygon) {
		getSurfaceProperties(isect->polyIndex, isect->uv, &isect->surfaceNormal, &textureCoord);
	}

	//Grab the 'base' diffuse color of the intersected object, and pass that to the
	//additional functions to add shading to it.
	struct color output = isect->end.diffuse;

	//getAmbient is a simple 'ambient occlusion' hack, works fine.
	struct color ambientColor = getAmbient(isect, &output);
	//getHighlights does specular + diffuse component
	struct color highlights = getHighlights(isect, &output, scene);

	//GetReflectsAndRefracts does just that
	struct color interacted = getReflectsAndRefracts(isect, &output, scene);
	
	struct color temp = mixColors(highlights, interacted, isect->end.reflectivity);
	return addColors(&temp, &ambientColor);
}

struct color getAmbientColor(struct lightRay *incidentRay) {
	struct vector unitDirection = normalizeVector(&incidentRay->direction);
	float t = 0.5 * (unitDirection.y + 1.0);
	struct color temp1 = colorCoef(1.0 - t, &(struct color){1.0, 1.0, 1.0, 0.0});
	struct color temp2 = colorCoef(t, &(struct color){0.5, 0.7, 1.0, 0.0});
	return addColors(&temp1, &temp2);
}

/**
 Recursive raytracer. (Unfinished)

 @param incidentRay Given light ray to cast into a scene
 @param scene Given scene to cast ray into
 @return Color based on the given ray and scene.
 */
struct color rayTrace(struct lightRay *incidentRay, struct world *scene) {
	//This is the start of the new rayTracer.
	//Start by getting the closest intersection point in scene for this given incidentRay.
	struct intersection closestIsect = getClosestIsect(incidentRay, scene);

	//Check if it hit something
	if (closestIsect.didIntersect) {
		return getLighting(&closestIsect, scene);
	} else {
		return getAmbientColor(incidentRay);
	}
}

struct vector randomInUnitSphere() {
	struct vector vec = (struct vector){0.0, 0.0, 0.0, false};
	do {
		vec = vectorMultiply(vectorWithPos(drand48(), drand48(), drand48()), 2.0);
		struct vector temp = vectorWithPos(1.0, 1.0, 1.0);
		vec = subtractVectors(&vec, &temp);
	} while (squaredVectorLength(&vec) >= 1.0);
	return vec;
}

bool lambertianScatter(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector temp = addVectors(&isect->hitPoint, &isect->surfaceNormal);
	struct vector rand = randomInUnitSphere();
	struct vector target = addVectors(&temp, &rand);
	
	struct vector target2 = subtractVectors(&isect->hitPoint, &target);
	
	*scattered = ((struct lightRay){isect->hitPoint, target2, rayTypeReflected, isect->end, 0});
	*attenuation = isect->end.diffuse;
	return true;
}

bool metallicScatter(struct intersection *isect, struct lightRay *ray, struct color *attenuation, struct lightRay *scattered) {
	struct vector reflected = reflect(&isect->ray.direction, &isect->surfaceNormal);
	*scattered = newRay(isect->hitPoint, reflected, rayTypeReflected);
	*attenuation = isect->end.diffuse;
	return (scalarProduct(&scattered->direction, &isect->surfaceNormal) > 0);
}

struct color pathTrace(struct lightRay *incidentRay, struct world *scene, int depth) {
	struct intersection rec = getClosestIsect(incidentRay, scene);
	
	struct coord textureCoord = {0.0, 0.0};
	if(rec.type == hitTypePolygon) {
		getSurfaceProperties(rec.polyIndex, rec.uv, &rec.surfaceNormal, &textureCoord);
	}
	
	if (rec.didIntersect) {
		struct lightRay scattered = {};
		struct color attenuation = {};
		
		if (depth < 5 && lambertianScatter(&rec, incidentRay, &attenuation, &scattered)) {
			struct color newColor = pathTrace(&scattered, scene, depth + 1);
			return multiplyColors(&attenuation, &newColor);
		} else {
			return (struct color){0.0, 0.0, 0.0, 0.0};
		}
		
	} else {
		return getAmbientColor(incidentRay);
	}
}
