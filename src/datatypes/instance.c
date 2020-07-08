//
//  instance.c
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "vector.h"
#include "../datatypes/transforms.h"
#include "instance.h"
#include "transforms.h"
#include "scene.h"

void addInstanceToScene(struct world *scene, struct instance instance) {
	if (scene->instanceCount == 0) {
		scene->instances = calloc(1, sizeof(struct instance));
	} else {
		scene->instances = realloc(scene->instances, (scene->instanceCount + 1) * sizeof(struct instance));
	}
	scene->instances[scene->instanceCount++] = instance;
}
