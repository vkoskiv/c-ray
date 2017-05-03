//
//  bbox.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct poly;

enum bboxAxis {
	X,
	Y,
	Z
};

struct boundingBox {
	struct vector start, end, midPoint;
};

struct boundingBox *computeBoundingBox(struct poly *polys, int count);

enum bboxAxis getLongestAxis(struct boundingBox *bbox);

bool rayIntersectWithAABB(struct boundingBox *box, struct lightRay *ray, double *t);
