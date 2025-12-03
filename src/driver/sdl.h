//
//  ui.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once
#include <stdbool.h>
#include <stddef.h>

struct texture;
struct cr_bitmap;
struct cr_tile;
struct cJSON;

// Returns NULL if we couldn't load the SDL2 lib and/or needed SDL symbols.
struct sdl_window *win_try_init(const struct cr_bitmap **buf);

enum input_event {
	ev_none = 0,
	ev_pause,
	ev_stop,
	ev_stop_nosave,
};
enum input_event win_update(struct sdl_window *w, const struct cr_tile *tiles, size_t tile_count);

void win_destroy(struct sdl_window *);
