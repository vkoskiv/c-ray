#pragma once

typedef struct vec2 {
	float x, y;
} vec2;


vec2 vec2_muls(vec2 v, float s);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);