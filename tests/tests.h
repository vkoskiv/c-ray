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

static test tests[] = {
	{"transforms::transpose", transform_transpose},
	{"transforms::multiply", transform_multiply},
	{"transforms::determinant", transform_determinant},
	{"transforms::determinant4x4", transform_determinant4x4},
	
	//{"transforms::rotateX", transform_rotate_X},
	//{"transforms::rotateY", transform_rotate_Y},
	//{"transforms::rotateZ", transform_rotate_Z},
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
	
	{"hashtable::mixed", hashtable_mixed},
	{"hashtable::fill", hashtable_fill},
};

#define testCount (sizeof(tests) / sizeof(test))
