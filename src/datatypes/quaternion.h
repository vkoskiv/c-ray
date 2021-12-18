//
//  quaternion.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 17/12/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct quaternion {
	float w, x, y, z;
};

struct euler_angles {
	float roll, pitch, yaw;
};

struct vector;

struct quaternion euler_to_quaternion(float roll, float pitch, float yaw);

void transform_vector_with_quaternion(struct vector *v, struct quaternion q);