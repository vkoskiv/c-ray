//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct crayOBJ;
struct light;
struct material;
struct sphere;
struct camera;
struct matrixTransform;
struct poly;
struct kdTreeNode;

//World
struct scene {
	
	struct color *ambientColor;
	
	struct crayOBJ *objs;
	int objCount;
	
	struct light *lights;
	int lightCount;
	
	struct material *materials;
	int materialCount;
	
	struct sphere *spheres;
	int sphereCount;
	
	//Currently only one camera supported
	struct camera *camera;
	int cameraCount;
	//FIXME: Store these in camera
	struct matrixTransform *camTransforms;
	int camTransformCount;
	
};

int testBuild(struct scene *scene, char *inputFileName);
