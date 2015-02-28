//
//  scene.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__scene__
#define __C_Ray__scene__

#include <stdio.h>
#include "CRay.h"
#include "sphere.h"
#include "poly.h"

//World
typedef struct {
	perspective viewPerspective;
	
	color *ambientColor;
	lightSource *lights;
	material *materials;
	sphereObject *spheres;
	polygonObject *polys;
	
	int height, width;
	
	int sphereAmount;
	int polygonAmount;
	int materialAmount;
	int lightAmount;
}world;

//This builds the scene. Will be replaced with a proper tokenizer 
int buildScene(bool randomGenerator, world *scene);

#endif /* defined(__C_Ray__scene__) */
