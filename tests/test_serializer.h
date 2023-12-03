//
//  test_serializer.h
//  c-ray
//
//  Created by Valtteri on 01/12/2023
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <c-ray/c-ray.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "../src/lib/renderer/renderer.h"
#include "../src/lib/protocol/protocol.h"
#include "../src/common/fileio.h"
#include "../src/vendored/cJSON.h"
#include "../src/driver/json_loader.h"
#include "../src/common/string.h"

void silence_stdout(int *bak, int *new) {
	fflush(stdout);
	*bak = dup(1);
	*new = open("/dev/null", O_WRONLY);
	dup2(*new, 1);
	close(*new);
}

void resume_stdout(int *bak, int *new) {
	(void)new;
	fflush(stdout);
	dup2(*bak, 1);
	close(*bak);
}

bool serializer_serialize(void) {

	file_data scene = file_load("input/hdr.json");
	test_assert(scene.items);

	cJSON *scene_json = cJSON_ParseWithLength((const char *)scene.items, scene.count);
	test_assert(scene_json);
	file_free(&scene);

	struct cr_renderer *ext = cr_new_renderer();
	test_assert(ext);

	cr_renderer_set_str_pref(ext, cr_renderer_asset_path, "input/");

	// Dirty hack, I should figure out a better way to control logging
	// Redirect stdout to /dev/null during parsing so we don't mess up the test output
	int bak, new;

	silence_stdout(&bak, &new);
	int ret = parse_json(ext, scene_json);
	resume_stdout(&bak, &new);

	test_assert(ret >= 0);
	cJSON_Delete(scene_json);

	struct renderer *r = (struct renderer *)ext;

	char *ser0 = serialize_renderer(r);
	test_assert(ser0);

	silence_stdout(&bak, &new);
	struct renderer *deserialized = deserialize_renderer(ser0);
	resume_stdout(&bak, &new);

	test_assert(deserialized);

	char *ser1 = serialize_renderer(deserialized);
	test_assert(stringEquals(ser0, ser1));

	free(ser0);
	free(ser1);

	cr_destroy_renderer(ext);

	return true;
}
