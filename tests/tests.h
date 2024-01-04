//
//  tests.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

static char *failed_expression;

//Test assert
#define test_assert(x) if (!(x)) { failed_expression = #x; return false; }

// And an approximate one for math stuff
#define _roughly_equals(a, b, tolerance) \
	do { \
		float expect_close_lhs = a; \
		float expect_close_rhs = b; \
		float expect_close_diff = (float)(expect_close_lhs) - (float)(expect_close_rhs); \
		if (fabsf(expect_close_diff) > tolerance) { \
			failed_expression = "roughly_equals (" #a " !≈ " #b ")";\
			return false; \
		} \
	} while (false)

#define roughly_equals(a, b) _roughly_equals(a, b, 0.0000005f)
#define very_roughly_equals(a, b) _roughly_equals(a, b, 0.01f)

#define vec_roughly_equals(veca, vecb) \
	do { \
		struct vector vec_a = veca; \
		struct vector vec_b = vecb; \
		roughly_equals(vec_a.x, vec_b.x); \
		roughly_equals(vec_a.y, vec_b.y); \
		roughly_equals(vec_a.z, vec_b.z); \
	} while (false)

// Testable modules
#include "test_textbuffer.h"
#include "test_transforms.h"
#include "test_vector.h"
#include "test_fileio.h"
#include "test_string.h"
#include "test_hashtable.h"
#include "test_base64.h"
#include "test_nodes.h"
#include "test_linked_list.h"
#include "test_parser.h"
#include "test_dyn_array.h"
#include "test_serializer.h"
#include "test_thread_pool.h"

typedef struct {
	char *test_name;
	bool (*func)(void);
} test;

static test tests[] = {
	{"vector::vecZero", vector_vecZero},
	{"vector::add", vector_vecAdd},
	{"vector::subtract", vector_vecSub},
	{"vector::multiply", vector_vecMul},
	{"vector::dot", vector_dot},
	{"vector::scale", vector_vecScale},
	{"vector::cross", vector_vecCross},
	{"vector::min", vector_vecMin},
	{"vector::max", vector_vecMax},
	{"vector::vecLengthSquared", vector_vecLengthSquared},
	{"vector::vecLength", vector_vecLength},
	{"vector::vecNormalize", vector_vecNormalize},
	{"vector::getmidpoint", vector_getMidPoint},
	{"vector::negate", vector_vecNegate},
	{"vector::baseWithVec", vector_baseWithVec},
	{"vector::vecEquals", vector_vecequals},
	{"vector::randomOnUnitSphere", vector_random_on_sphere},
	{"vector::reflect", vector_reflect},
	
	{"transforms::transpose", transform_transpose},
	{"transforms::multiply", transform_multiply},
	{"transforms::determinant", transform_determinant},
	{"transforms::determinant4x4", transform_determinant4x4},
	{"transforms::rotateX", transform_rotate_X},
	{"transforms::rotateY", transform_rotate_Y},
	{"transforms::rotateZ", transform_rotate_Z},
	{"transforms::translateX", transform_translate_X},
	{"transforms::translateY", transform_translate_Y},
	{"transforms::translateZ", transform_translate_Z},
	{"transforms::translateAll", transform_translate_all},
	{"transforms::scaleX", transform_scale_X},
	{"transforms::scaleY", transform_scale_Y},
	{"transforms::scaleZ", transform_scale_Z},
	{"transforms::scaleAll", transform_scale_all},
	{"transforms::inverse", transform_inverse},
	{"transforms::equal", matrix_equal},
	
	{"textbuffer::textview", textbuffer_textview},
	{"textbuffer::tokenizer", textbuffer_tokenizer},
	{"textbuffer::multispace", textbuffer_multispace},
	{"textbuffer::trailing_space", textbuffer_trailing_space},
	{"textbuffer::new", textbuffer_new},
	{"textbuffer::gotoline", textbuffer_gotoline},
	{"textbuffer::peekline", textbuffer_peekline},
	{"textbuffer::nextline", textbuffer_nextline},
	{"textbuffer::previousline", textbuffer_previousline},
	{"textbuffer::peeknextline", textbuffer_peeknextline},
	{"textbuffer::firstline", textbuffer_firstline},
	{"textbuffer::currentline", textbuffer_currentline},
	{"textbuffer::lastline", textbuffer_lastline},
	
	{"fileio::humanFileSize", fileio_humanFileSize},
	{"fileio::getFileName", fileio_getFileName},
	{"fileio::getFilePath", fileio_getFilePath},
	
	{"string::stringEquals", string_stringEquals},
	{"string::stringContains", string_stringContains},
	{"string::copyString", string_copyString},
	{"string::concatString", string_concatString},
	{"string::lowerCase", string_lowerCase},
	{"string::startsWith", string_startsWith},
	{"string::endsWith", string_endsWith},
	
	{"hashtable::mixed", hashtable_mixed},
	{"hashtable::fill", hashtable_fill},
	
	{"base64::basic", base64_basic},
	{"base64::padding_2", base64_padding_2},
	{"base64::padding_1", base64_padding_1},
	{"base64::padding_0", base64_padding_0},
	{"base64::varying", base64_varying},
	
	{"mathnode::add", mathnode_add},
	{"mathnode::subtract", mathnode_subtract},
	{"mathnode::multiply", mathnode_multiply},
	{"mathnode::divide", mathnode_divide},
	{"mathnode::power", mathnode_power},
	{"mathnode::log", mathnode_log},
	{"mathnode::squareroot", mathnode_squareroot},
	{"mathnode::invsquareroot", mathnode_invsquareroot},
	{"mathnode::absolute", mathnode_absolute},
	{"mathnode::min", mathnode_min},
	{"mathnode::max", mathnode_max},
	{"mathnode::lessthan", mathnode_lessthan},
	{"mathnode::greaterthan", mathnode_greaterthan},
	{"mathnode::sign", mathnode_sign},
	{"mathnode::compare", mathnode_compare},
	{"mathnode::round", mathnode_round},
	{"mathnode::floor", mathnode_floor},
	{"mathnode::ceil", mathnode_ceil},
	{"mathnode::truncate", mathnode_truncate},
	{"mathnode::fraction", mathnode_fraction},
	{"mathnode::modulo", mathnode_modulo},
	{"mathnode::sine", mathnode_sine},
	{"mathnode::cosine", mathnode_cosine},
	{"mathnode::tangent", mathnode_tangent},
	{"mathnode::toradians", mathnode_toradians},
	{"mathnode::todegrees", mathnode_todegrees},
	
	{"vecmath::vecAdd", vecmath_vecAdd},
	{"vecmath::vecSubtract", vecmath_vecSubtract},
	{"vecmath::vecMultiply", vecmath_vecMultiply},
	{"vecmath::vecDot", vecmath_vecDot},
	{"vecmath::vecCross", vecmath_vecCross},
	{"vecmath::vecNormalize", vecmath_vecNormalize},
	{"vecmath::vecReflect", vecmath_vecReflect},
	{"vecmath::vecLength", vecmath_vecLength},
	{"vecmath::vecAbs", vecmath_vecAbs},
	{"vecmath::vecScale", vecmath_vecScale},
	
	{"map_range::map", map_range},

	{"linked_list::basic", llist_basic},
	{"linked_list::remove_cb", llist_remove_cb},
	{"linked_list::remove_after_empty", llist_remove_after_empty},

	{"parser::parser_color_rgb", parser_color_rgb},
	{"parser::parser_color_array", parser_color_array},
	{"parser::parser_color_blackbody", parser_color_blackbody},
	{"parser::parser_color_hsl", parser_color_hsl},
	{"dyn_array::basic", dyn_array_basic},
	{"dyn_array::linear_grow", dyn_array_linear_grow},
	{"dyn_array::custom", dyn_array_custom},
	{"dyn_array::trim", dyn_array_trim},
	{"dyn_array::trim_expand", dyn_array_trim_expand},
	{"dyn_array::copy", dyn_array_copy},
	{"dyn_array::join", dyn_array_join},

	{"serializer::serialize", serializer_serialize},

	{"threadpool::basic", test_thread_pool},
};

#define testCount (sizeof(tests) / sizeof(test))
