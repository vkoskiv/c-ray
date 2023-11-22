//
//  ui.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once
#include <stdbool.h>
#include <stddef.h>

struct texture;
struct cr_tile;
struct cJSON;

struct sdl_prefs {
	bool enabled;
	bool fullscreen;
	bool borderless;
	float scale;
};

// Returns NULL if we couldn't load the SDL2 lib and/or needed SDL symbols.
struct sdl_window *win_try_init(struct sdl_prefs *prefs, int width, int height);
struct sdl_prefs sdl_parse(const struct cJSON *data);

struct input_state {
	bool pause_render;
	bool stop_render;
	bool should_save;
};
struct input_state win_update(struct sdl_window *w, const struct cr_tile *tiles, size_t tile_count, const struct texture *t);

void win_destroy(struct sdl_window *);
