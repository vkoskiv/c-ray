//
//  lightRay.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
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
	struct vector start;
	struct vector direction;
	enum type rayType;
	struct material currentMedium;
	int remainingInteractions; //Reflections or refractions
};

struct lightRay newRay(struct vector start, struct vector direction, enum type rayType);
