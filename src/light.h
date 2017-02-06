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
#include "vector.h"
#include "color.h"

//Light source
typedef struct {
    vector pos;
    float radius;
    color intensity;
}light;

#endif /* light_h */
