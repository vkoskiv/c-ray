#pragma once

#include "vec3.h"

typedef union
{
	float data[3*3];
	vec3 v[3];
	float e[3][3];
} mat3;

static vec3 mat3_mul_vec3(mat3 m, vec3 v)
{
	vec3 dst;
	dst.x = m.e[0][0] * v.x + m.e[1][0] * v.y + m.e[2][0] * v.z;
	dst.y = m.e[0][1] * v.x + m.e[1][1] * v.y + m.e[2][1] * v.z;
	dst.z = m.e[0][2] * v.x + m.e[1][2] * v.y + m.e[2][2] * v.z;
	return dst;
}

static mat3 mat3_transpose(mat3 m)
{
	return (mat3){
		m.e[0][0], m.e[1][0], m.e[2][0],
		m.e[0][1], m.e[1][1], m.e[2][1],
		m.e[0][2], m.e[1][2], m.e[2][2],
	};
}
/*
static mat3 mat3_transpose(mat3 m)
{
	return (mat3) {
		m.e[0][0], m.e[0][1], m.e[0][2],
		m.e[1][0], m.e[1][1], m.e[1][2],
		m.e[2][0], m.e[2][1], m.e[2][2],
	};
}*/
