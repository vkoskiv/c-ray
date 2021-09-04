//
//  spline.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 01/09/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "spline.h"

// This is actually a cubic bézier curve.

struct spline {
	struct vector a;
	struct vector b;
	struct vector c;
	struct vector d;
};

float lerp(const float a, const float b, const float t) {
	return (1.0f - t) * a + t * b;
}

struct vector vec_lerp(const struct vector a, const struct vector b, const float t) {
	return (struct vector){
		lerp(a.x, b.x, t),
		lerp(a.y, b.y, t),
		lerp(a.z, b.z, t)
	};
}

struct spline *spline_new(const struct vector a, const struct vector b, const struct vector c, const struct vector d) {
	struct spline *new = calloc(1, sizeof(*new));
	*new = (struct spline){ a, b, c, d };
	return new;
}

// Fairly intuitive, but this is called "De Casteljau's Algorithm", for those fancy math types.
struct vector spline_at(const struct spline *s, const float t) {
	const struct vector a_b = vec_lerp(s->a, s->b, t);
	const struct vector b_c = vec_lerp(s->b, s->c, t);
	const struct vector c_d = vec_lerp(s->c, s->d, t);
	
	const struct vector ab_bc = vec_lerp(a_b, b_c, t);
	const struct vector bc_cd = vec_lerp(b_c, c_d, t);
	
	return vec_lerp(ab_bc, bc_cd, t);
}

void spline_destroy(struct spline *spline) {
	if (spline) {
		free(spline);
	}
}
