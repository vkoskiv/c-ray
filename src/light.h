//
//  light.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 11/10/15.
//  Copyright © 2015 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Light source
struct light {
	struct vector pos;
	double power;
	struct color diffuse;
};

struct light newLight(struct vector pos, double radius, struct color diffuse);
