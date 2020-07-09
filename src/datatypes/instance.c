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
		scene->instances = calloc(1, sizeof(*scene->instances));
	} else {
		scene->instances = realloc(scene->instances, (scene->instanceCount + 1) * sizeof(*scene->instances));
	}
	scene->instances[scene->instanceCount++] = instance;
}

void addSphereInstanceToScene(struct world *scene, struct instance instance) {
	if (scene->sphereInstanceCount == 0) {
		scene->sphereInstances = calloc(1, sizeof(*scene->sphereInstances));
	} else {
		scene->sphereInstances = realloc(scene->sphereInstances, (scene->sphereInstanceCount + 1) * sizeof(*scene->sphereInstances));
	}
	scene->sphereInstances[scene->sphereInstanceCount++] = instance;
}
