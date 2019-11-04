#include "vec2.h"

vec2 vec2_muls(vec2 v, float s)
{
	return (vec2) { v.x * s, v.y * s };
}

vec2 vec2_add(vec2 v1, vec2 v2)
{
	return (vec2) { v1.x + v2.x, v1.y + v2.y };
}

vec2 vec2_sub(vec2 v1, vec2 v2)
{
	return (vec2) { v1.x - v2.x, v1.y - v2.y };
}