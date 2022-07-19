//
//  test_parser.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 17/07/2022.
//  Copyright © 2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../src/datatypes/color.h"
#include "../src/utils/loaders/sceneloader.h"

bool parser_color_rgb(void) {

	cJSON *data = NULL;
	struct color c = { 0 };

	data = cJSON_Parse("{}");
	c = parseColor(data);
	roughly_equals(c.red,   0.0f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue,  0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	data = cJSON_Parse("{\"r\": 0.1, \"g\": 0.2, \"b\": 0.3, \"a\": 0.4}");
	c = parseColor(data);
	roughly_equals(c.red,   0.1f);
	roughly_equals(c.green, 0.2f);
	roughly_equals(c.blue,  0.3f);
	roughly_equals(c.alpha, 0.4f);
	cJSON_Delete(data);

	data = cJSON_Parse("{\"g\": 0.2, \"b\": 0.3, \"a\": 0.4}");
	c = parseColor(data);
	roughly_equals(c.red,   0.0f);
	roughly_equals(c.green, 0.2f);
	roughly_equals(c.blue,  0.3f);
	roughly_equals(c.alpha, 0.4f);
	cJSON_Delete(data);

	data = cJSON_Parse("{\"r\": 0.1, \"b\": 0.3, \"a\": 0.4}");
	c = parseColor(data);
	roughly_equals(c.red,   0.1f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue,  0.3f);
	roughly_equals(c.alpha, 0.4f);
	cJSON_Delete(data);

	data = cJSON_Parse("{\"r\": 0.1, \"g\": 0.2, \"a\": 0.4}");
	c = parseColor(data);
	roughly_equals(c.red,   0.1f);
	roughly_equals(c.green, 0.2f);
	roughly_equals(c.blue,  0.0f);
	roughly_equals(c.alpha, 0.4f);
	cJSON_Delete(data);

	data = cJSON_Parse("{\"r\": 0.1, \"g\": 0.2, \"b\": 0.3}");
	c = parseColor(data);
	roughly_equals(c.red,   0.1f);
	roughly_equals(c.green, 0.2f);
	roughly_equals(c.blue,  0.3f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	return true;
}

bool parser_color_array(void) {

	cJSON *data = NULL;
	struct color c = { 0 };

	data = cJSON_Parse("[]");
	c = parseColor(data);
	roughly_equals(c.red,   0.0f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue,  0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	data = cJSON_Parse("[0.1, 0.2, 0.3, 0.4]");
	c = parseColor(data);
	roughly_equals(c.red, 0.1f);
	roughly_equals(c.green, 0.2f);
	roughly_equals(c.blue,  0.3f);
	roughly_equals(c.alpha, 0.4f);
	cJSON_Delete(data);

	data = cJSON_Parse("[0.2, 0.3, 0.4]");
	c = parseColor(data);
	roughly_equals(c.red, 0.2f);
	roughly_equals(c.green, 0.3f);
	roughly_equals(c.blue,  0.4f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	data = cJSON_Parse("[0.3, 0.4]");
	c = parseColor(data);
	roughly_equals(c.red, 0.3f);
	roughly_equals(c.green, 0.4f);
	roughly_equals(c.blue,  0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	data = cJSON_Parse("[0.4]");
	c = parseColor(data);
	roughly_equals(c.red, 0.4f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue,  0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	return true;
}

bool parser_color_blackbody(void) {
	struct cJSON *data = NULL;
	struct color c = { 0 };

	data = cJSON_Parse("{\"blackbody\": 6000}");
	c = parseColor(data);
	very_roughly_equals(c.red, 1.0f);
	very_roughly_equals(c.green, 0.96f);
	very_roughly_equals(c.blue, 0.92f);
	very_roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	return true;
}

bool parser_color_hsl(void) {
	struct cJSON *data = NULL;
	struct color c = { 0 };

	// Lightness 1
	data = cJSON_Parse("{\"h\": 0, \"s\": 100, \"l\": 100}");
	c = parseColor(data);
	roughly_equals(c.red, 1.0f);
	roughly_equals(c.green, 1.0f);
	roughly_equals(c.blue, 1.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	// Lightness 0
	data = cJSON_Parse("{\"h\": 0, \"s\": 100, \"l\": 0}");
	c = parseColor(data);
	roughly_equals(c.red, 0.0f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue, 0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	// Red
	data = cJSON_Parse("{\"h\": 0, \"s\": 100, \"l\": 50}");
	c = parseColor(data);
	roughly_equals(c.red, 1.0f);
	roughly_equals(c.green, 0.0f);
	roughly_equals(c.blue, 0.0f);
	roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	// Green
	data = cJSON_Parse("{\"h\": 120, \"s\": 100, \"l\": 50}");
	c = parseColor(data);
	very_roughly_equals(c.red, 0.0f);
	very_roughly_equals(c.green, 1.0f);
	very_roughly_equals(c.blue, 0.0f);
	very_roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	// Blue
	data = cJSON_Parse("{\"h\": 240, \"s\": 100, \"l\": 50}");
	c = parseColor(data);
	very_roughly_equals(c.red, 0.0f);
	very_roughly_equals(c.green, 0.0f);
	very_roughly_equals(c.blue, 1.0f);
	very_roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	// Should be red again
	data = cJSON_Parse("{\"h\": 360, \"s\": 100, \"l\": 50}");
	c = parseColor(data);
	very_roughly_equals(c.red, 1.0f);
	very_roughly_equals(c.green, 0.0f);
	very_roughly_equals(c.blue, 0.0f);
	very_roughly_equals(c.alpha, 1.0f);
	cJSON_Delete(data);

	return true;
}