//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "vector.h"
#include "material.h"

enum type {
	rayTypeIncident,
	rayTypeScattered,
	rayTypeReflected,
	rayTypeRefracted
};

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
	enum type rayType;
	int remainingInteractions; //Reflections or refractions
};

struct lightRay newRay(struct vector start, struct vector direction, enum type rayType);

struct vector alongRay(struct lightRay ray, float t);
