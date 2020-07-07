//
//  instance.h
//  C-ray
//
//  Created by Valtteri on 23.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct world;

struct instance newInstance(struct vector *rot, struct vector *scale, struct vector *translate);

void addInstanceToScene(struct world *scene, struct instance instance);
