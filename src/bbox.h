//
//  bbox.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly;

struct boundingBox {
	struct vector start, end;
};

struct boundingBox *computeBoundingBox(struct poly *polys, int count);
