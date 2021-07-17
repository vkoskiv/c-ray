//
//  color.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "color.h"

//Some standard colours
const struct color redColor =   {1.0f, 0.0f, 0.0f, 1.0f};
const struct color greenColor = {0.0f, 1.0f, 0.0f, 1.0f};
const struct color blueColor =  {0.0f, 0.0f, 1.0f, 1.0f};
const struct color blackColor = {0.0f, 0.0f, 0.0f, 1.0f};
const struct color grayColor =  {0.5f, 0.5f, 0.5f, 1.0f};
const struct color whiteColor = {1.0f, 1.0f, 1.0f, 1.0f};
const struct color clearColor = {0.0f, 0.0f, 0.0f, 0.0f};

//Colors for the SDL elements
const struct color backgroundColor = {0.1921568627f, 0.2f, 0.2117647059f, 1.0f};
const struct color frameColor = {1.0f, 0.5f, 0.0f, 1.0f};
const struct color progColor  = {0.2549019608f, 0.4509803922f, 0.9607843137f, 1.0f};

// This algorithm is from Tanner Helland:
// http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code/
struct color colorForKelvin(float kelvin) {
	struct color ret = {0};
	float temp = kelvin >= 40000.0f ? 40000.0f : kelvin;
	temp = temp / 100.0f;
	//Red
	if (temp <= 66.0f) {
		ret.red = 255.0f;
	} else {
		ret.red = temp - 60.0f;
		ret.red = 329.698727446f * powf(ret.red, -0.1332047592f);
		ret.red = ret.red < 0.0f ? 0.0f : ret.red;
		ret.red = ret.red > 255.0f ? 255.0f : ret.red;
	}
	
	//Green
	if (temp <= 66.0f) {
		ret.green = temp;
		ret.green = 99.4708025861f * logf(ret.green) - 161.1195681661f;
		ret.green = ret.green < 0.0f ? 0.0f : ret.green;
		ret.green = ret.green > 255.0f ? 255.0f : ret.green;
	} else {
		ret.green = temp - 60.0f;
		ret.green = 288.1221695283f * powf(ret.green, -0.0755148492f);
		ret.green = ret.green < 0.0f ? 0.0f : ret.green;
		ret.green = ret.green > 255.0f ? 255.0f : ret.green;
	}
	
	//Blue
	if (temp >= 66.0f) {
		ret.blue = 255.0f;
	} else {
		if (temp <= 19.0f) {
			ret.blue = 0.0f;
		} else {
			ret.blue = temp - 10.0f;
			ret.blue = 138.5177312231f * logf(ret.blue) - 305.0447927307f;
			ret.blue = ret.blue < 0.0f ? 0.0f : ret.blue;
			ret.blue = ret.blue > 255.0f ? 255.0f : ret.blue;
		}
	}
	
	return (struct color){ret.red / 255.0f, ret.green / 255.0f, ret.blue / 255.0f, 0};
}
