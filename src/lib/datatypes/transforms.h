//
//  transforms.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 07/02/2017.
//  Copyright © 2017-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>

/*
 C-ray's matrices use a *row-major* notation.
 _______________
 | 00 01 02 03 |
 | 10 11 12 13 |
 | 20 21 22 23 |
 | 30 31 32 33 |
 ---------------
 */

struct matrix4x4 {
	float mtx[4][4];
};

//Reference: http://tinyurl.com/ho6h6mr
struct transform {
	struct matrix4x4 A;
	struct matrix4x4 Ainv;
};

struct material;
struct vector;
struct boundingBox;
struct lightRay;

float deg_to_rad(float degrees);
float rad_to_deg(float radians);

//Transform types
struct transform tform_new_scale3(float x, float y, float z);
struct transform tform_new_scale(float scale);
struct transform tform_new_translate(float x, float y, float z);
struct transform tform_new_rot_x(float radians);
struct transform tform_new_rot_y(float radians);
struct transform tform_new_rot_z(float radians);
struct transform tform_new_rot(float roll, float pitch, float yaw);
struct transform tform_new(void);

struct matrix4x4 mat_invert(struct matrix4x4);
struct matrix4x4 mat_transpose(struct matrix4x4);
struct matrix4x4 mat_mul(struct matrix4x4 A, struct matrix4x4 B); //FIXME: Maybe don't expose this.
struct matrix4x4 mat_abs(struct matrix4x4);
struct matrix4x4 mat_id(void);
bool mat_eq(struct matrix4x4, struct matrix4x4);

void tform_point(struct vector *vec, struct matrix4x4);
void tform_vector(struct vector *vec, struct matrix4x4);
void tform_vector_transpose(struct vector *vec, struct matrix4x4);
void tform_bbox(struct boundingBox *bbox, struct matrix4x4);
void tform_ray(struct lightRay *ray, struct matrix4x4);
