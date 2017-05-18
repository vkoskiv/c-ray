//
//  lightRay.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 18/05/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum type {
	rayTypeIncident,
	rayTypeReflected,
	rayTypeRefracted,
	rayTypeShadow
};

//Simulated light ray
struct lightRay {
	struct vector start;
	struct vector direction;
	enum type rayType;
	struct material currentMedium;
	int remainingInteractions; //Reflections or refractions
};
