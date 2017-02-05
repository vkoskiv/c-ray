//
//  CRay.h
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#ifndef ____CRay__
#define ____CRay__

#include "includes.h"
#include "filehandler.h"
#include "errorhandler.h"
#include "vector.h"
#include "color.h"
#include "sphere.h"
#include "scene.h"
#include "display.h"

//Some macros
#define PIOVER180 0.017453292519943295769236907684886
#define PI        3.141592653589793238462643383279502
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

#define kFrameCount 1

//Generates a random value between a given range
float randRange(float a, float b);
//Converts degrees to radians
double rads(double angle);

#endif /* defined(____CRay__) */
