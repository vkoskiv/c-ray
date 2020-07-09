//
//  instance.h
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "transforms.h"

struct world;
struct matrix4x4;

struct instance {
	struct transform composite;
	void *object;
};

void addInstanceToScene(struct world *scene, struct instance instance);
void addSphereInstanceToScene(struct world *scene, struct instance instance);
