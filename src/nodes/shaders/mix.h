//
//  mix.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct bsdfNode *newMix(struct world *world, struct bsdfNode *A, struct bsdfNode *B, struct valueNode *factor);
