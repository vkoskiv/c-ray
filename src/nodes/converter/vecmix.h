//
//  vecmix.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/12/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

const struct vectorNode *new_vec_mix(const struct node_storage *s, const struct vectorNode *A, const struct vectorNode *B, const struct valueNode *factor);
