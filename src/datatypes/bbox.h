//
//  bbox.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <float.h>
#include "../datatypes/vector.h"

/// Bounding box for a given set of primitives
struct boundingBox {
    struct vector min, max;
};

static const struct boundingBox emptyBBox = {
	.min = {  FLT_MAX,  FLT_MAX,  FLT_MAX },
	.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX }
};

static inline float bboxHalfArea(const struct boundingBox *bbox) {
	vector extent = vecSub(bbox->max, bbox->min);
	return extent.x * (extent.y + extent.z) + extent.y * extent.z;
}

static inline void extendBBox(struct boundingBox *dst, const struct boundingBox *src) {
	dst->min = vecMin(dst->min, src->min);
	dst->max = vecMax(dst->max, src->max);
}

static inline vector bboxCenter(const struct boundingBox* bbox) {
    return vecScale(vecAdd(bbox->max, bbox->min), 0.5f);
}
