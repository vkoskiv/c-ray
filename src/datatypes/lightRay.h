//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum type {
	rayTypeIncident,
	rayTypeScattered,
	rayTypeReflected,
	rayTypeRefracted
};

//Simulated light ray
struct lightRay {
	vec3 start;
	vec3 direction;
	enum type rayType;
	Material *currentMedium;
	int remainingInteractions; //Reflections or refractions
};

struct lightRay newRay(vec3 start, vec3 direction, enum type rayType);
