//
//  quaternion.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 17/12/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include <math.h>
#include "quaternion.h"
#include "vector.h"

struct quaternion euler_to_quaternion(float roll, float pitch, float yaw) {
	const float cy = cosf(yaw * 0.5f);
	const float sy = sinf(yaw * 0.5f);
	const float cp = cosf(pitch * 0.5f);
	const float sp = sinf(pitch * 0.5f);
	const float cr = cosf(roll * 0.5f);
	const float sr = sinf(roll * 0.5f);

	return (struct quaternion){
		cr * cp * cy + sr * sp * sy,
		sr * cp * cy - cr * sp * sy,
		cr * sp * cy + sr * cp * sy,
		cr * cp * sy - sr * sp * cy,
	};
}

void transform_vector_with_quaternion(struct vector *v, struct quaternion q) {
	struct vector u = (struct vector){ q.x, q.y, q.z };
	float s = q.w;
	struct vector a = vec_scale(u, 2.0f * vec_dot(u, *v));
	struct vector b = vec_scale(*v, s * s - vec_dot(u, u));
	struct vector c = vec_scale(vec_cross(u, *v), 2.0f * s);
	struct vector temp = vec_add(a, vec_add(b, c));
	*v = temp;
}