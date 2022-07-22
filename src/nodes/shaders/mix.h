//
//  mix.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 30/11/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct bsdfNode *newMix(const struct node_storage *s, const struct bsdfNode *A, const struct bsdfNode *B, const struct valueNode *factor);
