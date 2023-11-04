//
//  ui.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderer;
struct texture;
struct sdl_prefs;

// Returns NULL if we couldn't load the SDL2 lib and/or needed SDL symbols.
struct sdl_window *win_try_init(struct sdl_prefs *prefs, int width, int height);
void win_update(struct sdl_window *w, struct renderer *r, struct texture *t);
void win_check_keyboard(struct sdl_window *w, struct renderer *r);
void win_destroy(struct sdl_window *);
