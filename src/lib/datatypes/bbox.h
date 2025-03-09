//
//  bbox.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/04/2017.
//  Copyright © 2017-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../includes.h"
#include <float.h>
#include <common/vector.h>
#include <common/transforms.h>

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

static inline void tform_bbox(struct boundingBox *bbox, const struct matrix4x4 m) {
	struct matrix4x4 abs = mat_abs(m);
	struct vector center = vec_scale(vec_add(bbox->min, bbox->max), 0.5f);
	struct vector halfExtents = vec_scale(vec_sub(bbox->max, bbox->min), 0.5f);
	tform_vector(&halfExtents, abs);
	tform_point(&center, abs);
	bbox->min = vec_sub(center, halfExtents);
	bbox->max = vec_add(center, halfExtents);
}
