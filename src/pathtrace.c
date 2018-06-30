//
//  pathtrace.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "pathtrace.h"

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

//#define SMOOTH
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

	//*calculatedNormal = normalizeVector(calculatedNormal);
	
#ifdef UV
	//Texture coords
	struct coord uv1 = textureArray[polygonArray[polyIndex].textureIndex[0]];
	struct coord uv2 = textureArray[polygonArray[polyIndex].textureIndex[1]];
	struct coord uv3 = textureArray[polygonArray[polyIndex].textureIndex[2]];
	
	//uv is our barycentric coordinate from intersection
	double s = uv.x;
	double t = uv.y;
	
	double tmp = (1 - s - t);
	struct coord comp1 = coordScale(tmp, &uv1);
	struct coord comp2 = coordScale(s, &uv2);
	struct coord comp3 = coordScale(t, &uv3);
	
	struct coord temp = addCoords(&comp1, &comp2);
	struct coord final = addCoords(&temp, &comp3);
	//textureCoord *should* be the X,Y value we take from our texture image
	*textureCoord = uvFromValues(final.x, final.y);
	
	// (1 - uv.x - uv.y) * st0 + uv.x * st1 + uv.y * st2;
	
	
	/*
	 @vkoskiv the important bit is
	 "you use the barycentric coords (s, t, 1 - s - t) to weight the uvs (or any other data) at the vertices, to get the (u, v) at your point in the triangle"
	 just interpolate sUV1 + tUV2 + (1-s-t)*UV3 = final UV, if the UVs of the tree vertices are UV1,2,3
	 no need to normalize the weights, because s + t + 1 - s - t = 1 trivially
	 */
	
	/* sUV1 + tUV2 + (1-s-t)*UV3 = final UV */
	
	
	// (1 - s - t) * uv1 + s * uv2 + t * uv3;
	
	// (1 - s - t) * uv1
	
	
	//double v = coordScale(s, &uv2);
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
	
	isect.surfaceNormal = normalizeVector(&isect.surfaceNormal);
	
	return isect;
}

struct color getAmbientColor(struct lightRay *incidentRay) {
	struct vector unitDirection = normalizeVector(&incidentRay->direction);
	float t = 0.5 * (unitDirection.y + 1.0);
	struct color temp1 = colorCoef(1.0 - t, &(struct color){1.0, 1.0, 1.0, 0.0});
	struct color temp2 = colorCoef(t, &(struct color){0.5, 0.7, 1.0, 0.0});
	return addColors(&temp1, &temp2);
}

/*struct color getPixel(int x, int y) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	output.red = mainRenderer.renderBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width)*3 + 0];
	output.green = mainRenderer.renderBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width)*3 + 1];
	output.blue = mainRenderer.renderBuffer[(x + (mainRenderer.image->size.height - y) * mainRenderer.image->size.width)*3 + 2];
	output.alpha = 1.0;
	return output;
}*/

struct color colorForUV(struct material mtl, struct coord uv) {
	struct color output = {0.0, 0.0, 0.0, 0.0};
	//We need to combine the given uv, material texture coordinates, and magic to resolve this color.
	
	int x = (int)uv.x;
	int y = (int)uv.y;
  
	output.red = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 0];
	output.green = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 1];
	output.blue = mtl.texture->imgData[(x + (*mtl.texture->height - y) * *mtl.texture->width)*3 + 2];
	
	return output;
}

struct color pathTrace(struct lightRay *incidentRay, struct world *scene, int depth) {
	struct intersection rec = getClosestIsect(incidentRay, scene);
	if (rec.didIntersect) {
		struct lightRay scattered = {};
		struct color attenuation = {};
		if (depth < scene->bounces && rec.end.bsdf(&rec, incidentRay, &attenuation, &scattered)) {
			struct color newColor = pathTrace(&scattered, scene, depth + 1);
			return multiplyColors(&attenuation, &newColor);
		} else {
			return (struct color){0.0, 0.0, 0.0, 0.0};
		}
	} else {
		return getAmbientColor(incidentRay);
	}
}
