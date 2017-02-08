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
#include "transforms.h"

//World
typedef struct {
	crayOBJ *objs;
	color *ambientColor;
	light *lights;
	material *materials;
	sphere *spheres;
	camera *camera;
	
	//Temporary
	int customVertexCount;
	
	poly *polys;
	
	int sphereAmount;
	int polygonAmount;
	int materialAmount;
	int lightAmount;
	int objCount;
}world;

int testBuild(world *scene, char *inputFileName);

#endif /* defined(__C_Ray__scene__) */
