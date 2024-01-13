//
//  color_ramp.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 12/01/2024.
//  Copyright © 2024 Valtteri Koskivuori. All rights reserved.
//

#include <stdio.h>
#include "../../../common/hashtable.h"
#include "../../../common/vector.h"
#include "../../datatypes/scene.h"
#include "../../../../include/c-ray/node.h"
#include "../colornode.h"

#include "color_ramp.h"

typedef struct ramp_element ramp_element;
dyn_array_def(ramp_element);

struct color_ramp_node {
	struct colorNode node;
	const struct valueNode *input_value;
	enum cr_color_mode color_mode;
	enum cr_interpolation interpolation;
	struct ramp_element_arr elements;
};

static bool compare(const void *A, const void *B) {
	const struct color_ramp_node *this = A;
	const struct color_ramp_node *other = B;
	bool elements_match = true;
	if (this->elements.count != other->elements.count) elements_match = false;
	for (size_t i = 0; i < this->elements.count && elements_match; ++i) {
		if (this->elements.items[i].position != other->elements.items[i].position) {
			elements_match = false;
			break;
		}
		struct cr_color a = this->elements.items[i].color;
		struct cr_color b = other->elements.items[i].color;
		if (a.r != b.r || a.g != b.g || a.b != b.b || a.a != b.a) {
			elements_match = false;
			break;
		}
	}
	return this->input_value == other->input_value &&
	this->color_mode == other->color_mode &&
	this->interpolation == other->interpolation &&
	elements_match;
}

static uint32_t hash(const void *p) {
	const struct color_ramp_node *this = p;
	uint32_t h = hashInit();
	h = hashBytes(h, &this->input_value, sizeof(this->input_value));
	h = hashBytes(h, &this->color_mode, sizeof(this->color_mode));
	h = hashBytes(h, &this->interpolation, sizeof(this->interpolation));
	for (size_t i = 0; i < this->elements.count; ++i)
		h = hashBytes(h, &this->elements.items[i], sizeof(this->elements.items[i]));
	return h;
}

// static void dump(const void *node, char *dumpbuf, int bufsize) {
// 	struct color_ramp_node *self = (struct color_ramp_node *)node;
// 	// FIXME
// 	(void)self;
// }

// TODO: This most certainly needs a bunch of tests to verify correctness
static struct color eval(const struct colorNode *node, sampler *sampler, const struct hitRecord *record) {
	const struct color_ramp_node *this = (const struct color_ramp_node *)node;
	const float pos = this->input_value->eval(this->input_value, sampler, record);
	
	if (this->elements.count == 1)
		return *(struct color *)&this->elements.items[0].color;

	if (pos <= this->elements.items[0].position)
		return *(struct color *)&this->elements.items[0].color;

	if (pos >= this->elements.items[this->elements.count - 1].position)
		return *(struct color *)&this->elements.items[this->elements.count - 1].color;
	
	struct ramp_element *left = NULL;
	for (size_t i = 0; i < this->elements.count; ++i) {
		if (this->elements.items[i].position <= pos) {
			left = &this->elements.items[i];
		}
	}
	struct ramp_element *right = NULL;
	for (size_t i = this->elements.count - 1; i; --i) {
		if (this->elements.items[i].position >= pos) {
			right = &this->elements.items[i];
		}
	}

	if (!left || !right) {
		logr(warning, "color_ramp: Missing ramp elements: %s %s\n", !left ? "left" : "", !right ? "right" : "");
		return (struct color){ 0 };
	}

	if (this->interpolation == cr_constant) {
		return *(struct color *)&left->color;
	}
	float t = inv_lerp(left->position, right->position, pos);
	return colorLerp(*(struct color *)&left->color, *(struct color *)&right->color, t);
}

const struct colorNode *new_color_ramp(const struct node_storage *s,
                                       const struct valueNode *input_value,
                                       enum cr_color_mode color_mode,
                                       enum cr_interpolation interpolation,
                                       struct ramp_element *elements,
                                       int element_count) {
	if (!elements || !element_count) {
		logr(warning, "color_ramp: No control points provided, bailing out\n");
		return newConstantTexture(s, g_pink_color);
	}
	struct ramp_element_arr element_arr;
	for (int i = 0; i < element_count; ++i) {
		ramp_element_arr_add(&element_arr, elements[i]);
	}
	// Validate mode, interpolation and elements first
	// Frankly, I don't even know what this mode does yet, need to look into that
	// FIXME: Support HSV and HSL modes
	if (color_mode != cr_mode_rgb) {
		logr(warning, "color_ramp: color mode %s not supported yet, setting to RGB",
			color_mode == cr_mode_hsv ? "HSV" : color_mode == cr_mode_hsl ? "HSL" : "RGB");
		color_mode = cr_mode_rgb;
	}
	// FIXME: Support all interpolation modes
	if (interpolation != cr_linear && interpolation != cr_constant) {
		logr(warning, "color_ramp: Only linear and constant interpolations are supported, setting to linear\n");
		interpolation = cr_linear;
	}
	// Now validate all the control points
	for (size_t i = 0; i < element_arr.count; ++i) {
		struct ramp_element *e = &element_arr.items[i];
		if (e->position < 0.0f || e->position > 1.0f) {
			logr(warning, "Invalid control point position: %.3f, clamping\n", e->position);
			if (e->position > 1.0f) e->position = 1.0f;
			if (e->position < 0.0f) e->position = 0.0f;
		}
	}
	HASH_CONS(s->node_table, hash, struct color_ramp_node, {
		// TODO: If the input is constant, we can probably evaluate this at setup time, right?
		.input_value = input_value ? input_value : newConstantValue(s, 0.0f),
		.color_mode = color_mode,
		.interpolation = interpolation,
		.elements = element_arr,
		.node = {
			.eval = eval,
			.base = { .compare = compare, .dump = NULL }
		}
	});
}
