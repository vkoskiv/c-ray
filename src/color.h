//
//  color.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef __C_Ray__color__
#define __C_Ray__color__

#include "includes.h"

//Color
typedef struct {
	float red, green, blue, alpha;
}color;

//Some standard colours
/*color redColor =   {1.0, 0.0, 0.0};
color greenColor = {0.0, 1.0, 0.0};
color blueColor =  {0.0, 0.0, 1.0};
color blackColor = {0.0, 0.0, 0.0};
color whiteColor = {1.0, 1.0, 1.0};*/

//Multiply two colors and return the resulting color
color multiplyColors(color *c1, color *c2);

//Add two colors and return the resulting color
color addColors(color *c1, color *c2);

//Multiply a color by a coefficient and return the resulting color
color colorCoef(double coef, color *c);

#endif /* defined(__C_Ray__color__) */
