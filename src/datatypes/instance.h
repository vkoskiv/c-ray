//
//  instance.h
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;
struct matrix4x4;

struct instance {
	struct transform composite;
	int meshIdx;
};

void addInstanceToScene(struct world *scene, struct instance instance);
