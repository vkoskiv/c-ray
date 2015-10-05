//
//  modeler.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 04/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__modeler__
#define __C_Ray__modeler__

#include "includes.h"
#include "vector.h"
#include "poly.h"

//Object, a cube (WIP)
typedef struct {
    vector pos;
    float edgeLength;
    int material;
}cubeObject;

polyMesh *buildPlane(vector *corner1, vector *corner2, vector *corner3, vector *corner4);

#endif /* defined(__C_Ray__modeler__) */
