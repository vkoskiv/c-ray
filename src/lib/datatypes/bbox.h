//
//  bbox.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../includes.h"
#include <float.h>
#include "../../common/vector.h"

/// Bounding box for a given set of primitives
struct boundingBox {
	struct vector min, max;
};

static const struct boundingBox emptyBBox = {
	.min = {  FLT_MAX,  FLT_MAX,  FLT_MAX },
	.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX }
};

static inline float bboxHalfArea(const struct boundingBox *bbox) {
	struct vector extent = vec_sub(bbox->max, bbox->min);
	return extent.x * (extent.y + extent.z) + extent.y * extent.z;
}

static inline void extendBBox(struct boundingBox *dst, const struct boundingBox *src) {
	dst->min = vec_min(dst->min, src->min);
	dst->max = vec_max(dst->max, src->max);
}

static inline struct vector bboxCenter(const struct boundingBox* bbox) {
	return vec_scale(vec_add(bbox->max, bbox->min), 0.5f);
}

static inline float bboxDiagonal(const struct boundingBox bbox) {
	struct vector extent = vec_sub(bbox.max, bbox.min);
	return vec_length(extent);
}

static inline float rayOffset(const struct boundingBox bbox) {
	return RAY_OFFSET_MULTIPLIER * bboxDiagonal(bbox);
}
