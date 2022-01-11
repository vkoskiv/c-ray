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
	const float cy = cos(yaw * 0.5f);
	const float sy = sin(yaw * 0.5f);
	const float cp = cos(pitch * 0.5f);
	const float sp = sin(pitch * 0.5f);
	const float cr = cos(roll * 0.5f);
	const float sr = sin(roll * 0.5f);

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
	struct vector a = vecScale(u, 2.0f * vecDot(u, *v));
	struct vector b = vecScale(*v, s * s - vecDot(u, u));
	struct vector c = vecScale(vecCross(u, *v), 2.0f * s);
	struct vector temp = vecAdd(a, vecAdd(b, c));
	*v = temp;
}