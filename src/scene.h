//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__scene__
#define __C_Ray__scene__

#include "includes.h"
#include "errorhandler.h"
#include "sphere.h"
#include "poly.h"
#include "camera.h"
#include "color.h"
#include "light.h"

//material
typedef struct {
    color diffuse;
    float reflectivity;
}material;

//World
typedef struct {
	color *ambientColor;
	lightSphere *lights;
	material *materials;
	sphereObject *spheres;
	polygonObject *polys;
	camera camera;
	
	int sphereAmount;
	int polygonAmount;
	int materialAmount;
	int lightAmount;
}world;

//This takes an input file, tokenizes it and applies it to a world object.
int buildScene(world *scene, char *inputFileName);

int loadOBJ(world *scene, char *inputFileName);

#endif /* defined(__C_Ray__scene__) */
