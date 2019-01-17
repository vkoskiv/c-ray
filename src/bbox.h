//
//  bbox.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
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

struct boundingBox *computeBoundingBox(int *polys, int count);

enum bboxAxis getLongestAxis(struct boundingBox *bbox);

bool rayIntersectWithAABB(struct boundingBox *box, struct lightRay *ray, double *t);
