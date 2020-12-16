//
//  add.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 17/12/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct bsdf *newAdd(struct world *world, struct bsdf *A, struct bsdf *B);
