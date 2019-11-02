#include "../includes.h"
#include "vec4.h"

/* Vector Functions */

vec4 vec4_mul(vec4 v1, vec4 v2)
{
	return (vec4) { v1.x* v2.x, v1.y* v2.y, v1.z* v2.z, v1.w * v2.w };
}

vec4 vec4_muls(vec4 v, float x)
{
	return (vec4) { v.x* x, v.y* x, v.z* x, v.w * x };
}

vec4 vec4_add(vec4 v1, vec4 v2)
{
	return (vec4) { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w };
}

float vec4_length(vec4 v)
{
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

float vec4_dot(vec4 v1, vec4 v2)
{
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

vec4 vec4_divs(vec4 v, float x)
{
	return (vec4) { v.x / x, v.y / x, v.z / x, v.w / x };
}

vec4 vec4_negate(vec4 v)
{
	return (vec4) { -v.x, -v.y, -v.z, -v.w };
}

vec4 vec4_normalize(vec4 v)
{
	float l = vec4_length(v);
	return (vec4) { v.x / l, v.y / l, v.z / l, v.w / l };
}

vec4 vec4_sub(vec4 v1, vec4 v2)
{
	return (vec4) { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w };
}

vec4 vec4_subs(vec4 v, float x)
{
	return (vec4) { v.x - x, v.y - x, v.z - x, v.w - x };
}

vec4 vec4_adds(vec4 v, float x)
{
	return (vec4) { v.x + x, v.y + x, v.z + x, v.w + x };
}

vec4 vec4_mix(vec4 x0, vec4 x1, float t)
{
	return vec4_add(vec4_muls(x0, 1.0f - t), vec4_muls(x1, t));
}