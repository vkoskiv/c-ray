//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once
#include "includes.h"
#include "errorhandler.h"
#include "sphere.h"
#include "poly.h"
#include "camera.h"
#include "color.h"
#include "light.h"
#include "transforms.h"

//World
typedef struct {
	color *ambientColor;
	
	crayOBJ *objs;
	int objCount;
	
	light *lights;
	int lightCount;
	
	material *materials;
	int materialCount;
	
	sphere *spheres;
	int sphereCount;
	
	//Currently only one camera supported
	camera *camera;
	int cameraCount;
	//FIXME: Store these in camera
	matrixTransform *camTransforms;
	int camTransformCount;
	
	//FIXME: TEMPORARY
	poly *customPolys;
	int customPolyCount;
	
}scene;

int testBuild(scene *scene, char *inputFileName);
scene *newScene();
