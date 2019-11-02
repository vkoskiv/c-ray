#pragma once

#include "vec3.h"

typedef union
{
	float3 v[3];
	float e[3][3];
} float3x3, mat3x3, mat3;

static vec3 mat3_mul_vec3(mat3 m, vec3 v)
{
	vec3 dst;
	dst.e[0] = m.e[0][0] * v.e[0] + m.e[1][0] * v.e[1] + m.e[2][0] * v.e[2];
	dst.e[1] = m.e[0][1] * v.e[0] + m.e[1][1] * v.e[1] + m.e[2][1] * v.e[2];
	dst.e[2] = m.e[0][2] * v.e[0] + m.e[1][2] * v.e[1] + m.e[2][2] * v.e[2];
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