//
//  light.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 11/10/15.
//  Copyright © 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef light_h
#define light_h

#include "includes.h"
#include "poly.h"
#include "color.h"

typedef struct {
    polygonObject p1;
    polygonObject p2;
    color intensity;
}lightPlane;

//Light source (point)
typedef struct {
    vector pos;
    float radius;
    color intensity;
}lightSphere;

#endif /* light_h */
