//
//  tests.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

static char *failed_expression;

// Testable modules
#include "test_textbuffer.h"
#include "test_transforms.h"
#include "test_vector.h"
#include "test_fileio.h"
#include "test_string.h"
#include "test_hashtable.h"
#include "test_mempool.h"
#include "test_base64.h"
#include "test_nodes.h"

static test tests[] = {
	{"vector::add", vector_vecAdd},
	{"vector::subtract", vector_vecSub},
	{"vector::multiply", vector_vecMul},
	{"vector::dot", vector_dot},
	{"vector::scale", vector_vecScale},
	{"vector::min", vector_vecMin},
	{"vector::max", vector_vecMax},
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
	
	{"hashtable::mixed", hashtable_mixed},
	{"hashtable::fill", hashtable_fill},
	/*{"mempool::bigalloc", mempool_bigalloc},
	{"mempool::tinyalloc8", mempool_tiny_8},
	{"mempool::tinyalloc16", mempool_tiny_16},
	{"mempool::tinyalloc32", mempool_tiny_32},
	{"mempool::tinyalloc64", mempool_tiny_64},
	{"mempool::tinyalloc128", mempool_tiny_128},
	{"mempool::tinyalloc256", mempool_tiny_256},
	{"mempool::tinyalloc512", mempool_tiny_512},
	{"mempool::tinyalloc1024", mempool_tiny_1024},
	{"mempool::tinyalloc2048", mempool_tiny_2048},
	{"mempool::tinyalloc4096", mempool_tiny_4096},*/
	
	{"base64::basic", base64_basic},
	{"base64::padding_2", base64_padding_2},
	{"base64::padding_1", base64_padding_1},
	{"base64::padding_0", base64_padding_0},
	{"mathnode::add", mathnode_add},
	{"mathnode::subtract", mathnode_subtract},
	{"mathnode::multiply", mathnode_multiply},
	{"mathnode::divide", mathnode_divide},
	{"mathnode::power", mathnode_power},
	{"mathnode::log", mathnode_log},
	{"mathnode::squareroot", mathnode_squareroot},
	{"mathnode::absolute", mathnode_absolute},
	{"mathnode::min", mathnode_min},
	{"mathnode::max", mathnode_max},
	{"mathnode::sine", mathnode_sine},
	{"mathnode::cosine", mathnode_cosine},
	{"mathnode::tangent", mathnode_tangent},
	{"mathnode::toradians", mathnode_toradians},
	{"mathnode::todegrees", mathnode_todegrees},
};

#define testCount (sizeof(tests) / sizeof(test))
