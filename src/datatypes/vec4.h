#pragma once

typedef union
{
	struct { float x, y, z, w; };
	struct { float r, g, b, a; };
	struct { float red, green, blue, alpha; };
	float e[4];
} vec4, float4;

static const vec4 VEC4_ZERO = { 0.0f, 0.0f, 0.0f, 0.0 };
static const vec4 VEC4_ONE = { 1.0f, 1.0f, 1.0f, 1.0f };

vec4 vec4_mix(vec4 x0, vec4 x1, float t);
vec4 vec4_mul(vec4 v1, vec4 v2);
vec4 vec4_muls(vec4 v, float x);
vec4 vec4_divs(vec4 v, float x);
vec4 vec4_add(vec4 v1, vec4 v2);
vec4 vec4_muls(vec4 v, float x);
vec4 vec4_negate(vec4 v);
vec4 vec4_normalize(vec4 v);
vec4 vec4_sub(vec4 v1, vec4 v2);
vec4 vec4_subs(vec4 v, float x);
vec4 vec4_adds(vec4 v, float x);
float vec4_length(vec4 v);
float vec4_dot(vec4 v1, vec4 v2);