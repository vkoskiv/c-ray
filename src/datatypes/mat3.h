#pragma once

#include "vec3.h"

typedef union
{
	float e[3][3];
	vec3 v[3];
} mat3;

mat3 mat3_transpose(mat3 m);
vec3 mat3_mul_vec3(mat3 m, vec3 v);