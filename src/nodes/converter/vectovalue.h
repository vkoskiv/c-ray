//
//  vectovalue.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 20/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

enum component {
	X,
	Y,
	Z,
	U,
	V,
	F,
};

const struct valueNode *newVecToValue(const struct node_storage *s, const struct vectorNode *vec, enum component component);