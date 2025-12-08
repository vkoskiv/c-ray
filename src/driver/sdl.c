//
//  sdl.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 19/02/2017.
//  Copyright © 2017-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "sdl.h"

#include "imagefile.h"
#include <common/texture.h>
#include <common/color.h>
#include <common/logging.h>
#include <common/cr_assert.h>
#include <common/logo.h>
#include <common/loaders/textureloader.h>
#include <common/vendored/cJSON.h>
#include <c-ray/c-ray.h>
#include <v.h>

#include "vendored/SDL2/SDL_render.h"
#include "vendored/SDL2/SDL_events.h"

struct sdl_syms {
	void *lib;
	int (*SDL_VideoInit)(const char *driver_name);
	void (*SDL_VideoQuit)(void);
	void (*SDL_Quit)(void);
	const char *(*SDL_GetError)(void);
	void (*SDL_SetWindowIcon)(SDL_Window *window, SDL_Surface *icon);
	void (*SDL_FreeSurface)(SDL_Surface *surface);
	SDL_Surface *(*SDL_CreateRGBSurfaceFrom)(void *pixels, int width, int height, int depth, int pitch,
											uint32_t rmask, uint32_t gmask, uint32_t bmask, uint32_t amask);
	SDL_Window *(*SDL_CreateWindow)(const char *title, int x, int y, int w, int h, uint32_t flags);
	SDL_Renderer *(*SDL_CreateRenderer)(SDL_Window *window, int index, uint32_t flags);
	SDL_Texture *(*SDL_CreateTexture)(SDL_Renderer *renderer, uint32_t format, int access, int w, int h);
	void (*SDL_DestroyTexture)(SDL_Texture *texture);
	void (*SDL_DestroyRenderer)(SDL_Renderer *renderer);
	void (*SDL_DestroyWindow)(SDL_Window *window);
	void (*SDL_RenderPresent)(SDL_Renderer *renderer);
	int (*SDL_RenderSetLogicalSize)(SDL_Renderer *renderer, int w, int h);
	int (*SDL_SetRenderDrawBlendMode)(SDL_Renderer *renderer, SDL_BlendMode blend_mode);
	int (*SDL_SetTextureBlendMode)(SDL_Texture *texture, SDL_BlendMode blend_mode);
	int (*SDL_RenderSetScale)(SDL_Renderer *renderer, float scaleX, float scaleY);
	int (*SDL_PollEvent)(SDL_Event *event);
	int (*SDL_UpdateTexture)(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch);
	int (*SDL_RenderCopy)(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);

};

static void *try_find_sdl2_lib(void) {
	char *candidates[] = {
		"libSDL2-2.0.so.0",
		"libSDL2-2.0.0.dylib",
		"/usr/local/lib/libSDL2.dylib",
		"SDL2.dll",
	};
	void *lib = NULL;
	for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
		if ((lib = v_mod_load(candidates[i])))
			return lib;
	}

	logr(debug, "Couldn't find SDL library: %s\n", v_mod_error());
	logr(debug, "Tried the following names: ");
	if (cr_log_level_get() == Debug) {
		for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i)
			logr(plain, "\"%s\" ", candidates[i]);
		logr(plain, "\n");
	}

	return NULL;
}

static struct sdl_syms try_get_sdl2_syms(void) {
	struct sdl_syms syms = { 0 };
	void *sdl2 = try_find_sdl2_lib();
	if (!sdl2)
		return syms;
	/*
		warning: ISO C forbids initialization between function pointer and ‘void *’ [-Wpedantic]
			=> We don't care about this in $CURRENT_YEAR :]
	*/
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic push
	syms = (struct sdl_syms){
		.lib = sdl2,
		.SDL_VideoInit              = v_mod_sym(sdl2, "SDL_VideoInit"),
		.SDL_VideoQuit              = v_mod_sym(sdl2, "SDL_VideoQuit"),
		.SDL_Quit                   = v_mod_sym(sdl2, "SDL_Quit"),
		.SDL_GetError               = v_mod_sym(sdl2, "SDL_GetError"),
		.SDL_SetWindowIcon          = v_mod_sym(sdl2, "SDL_SetWindowIcon"),
		.SDL_FreeSurface            = v_mod_sym(sdl2, "SDL_FreeSurface"),
		.SDL_CreateRGBSurfaceFrom   = v_mod_sym(sdl2, "SDL_CreateRGBSurfaceFrom"),
		.SDL_CreateWindow           = v_mod_sym(sdl2, "SDL_CreateWindow"),
		.SDL_CreateRenderer         = v_mod_sym(sdl2, "SDL_CreateRenderer"),
		.SDL_CreateTexture          = v_mod_sym(sdl2, "SDL_CreateTexture"),
		.SDL_DestroyTexture         = v_mod_sym(sdl2, "SDL_DestroyTexture"),
		.SDL_DestroyRenderer        = v_mod_sym(sdl2, "SDL_DestroyRenderer"),
		.SDL_DestroyWindow          = v_mod_sym(sdl2, "SDL_DestroyWindow"),
		.SDL_RenderPresent          = v_mod_sym(sdl2, "SDL_RenderPresent"),
		.SDL_RenderSetLogicalSize   = v_mod_sym(sdl2, "SDL_RenderSetLogicalSize"),
		.SDL_SetRenderDrawBlendMode = v_mod_sym(sdl2, "SDL_SetRenderDrawBlendMode"),
		.SDL_SetTextureBlendMode    = v_mod_sym(sdl2, "SDL_SetTextureBlendMode"),
		.SDL_RenderSetScale         = v_mod_sym(sdl2, "SDL_RenderSetScale"),
		.SDL_PollEvent              = v_mod_sym(sdl2, "SDL_PollEvent"),
		.SDL_UpdateTexture          = v_mod_sym(sdl2, "SDL_UpdateTexture"),
		.SDL_RenderCopy             = v_mod_sym(sdl2, "SDL_RenderCopy")
	};
	#pragma GCC diagnostic pop
	for (size_t i = 0; i < (sizeof(struct sdl_syms) / sizeof(void *)); ++i) {
		if (!((void **)&syms)[i]) {
			logr(warning, "sdl_syms[%zu] is NULL\n", i);
			v_mod_close(sdl2);
			return (struct sdl_syms){ 0 };
		}
	}
	return syms;
}

struct sdl_window {
	struct sdl_syms sdl2;
	SDL_Window *window;
	SDL_Renderer *renderer;

	const struct cr_bitmap **rbuf;
	struct texture *texture;
	SDL_Texture *texture_sdl;
	struct texture *overlay;
	SDL_Texture *overlay_sdl;
};


static void set_window_icon(struct sdl_window *w) {
#ifndef NO_LOGO
	struct texture *icon = tex_new(none, 0, 0, 0);
	int ret = load_texture("logo.h", (file_data){ .items = logo_png_data, .count = logo_png_data_len }, icon);
	if (ret) return;
	uint32_t rmask;
	uint32_t gmask;
	uint32_t bmask;
	uint32_t amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (icon->channels == 3) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (icon->channels == 3) ? 0 : 0xff000000;
#endif
	SDL_Surface *iconSurface = w->sdl2.SDL_CreateRGBSurfaceFrom(icon->data.byte_p,
														(int)icon->width,
														(int)icon->height,
														(int)icon->channels * 8,
														(int)(icon->channels * icon->width),
														rmask, gmask, bmask, amask);
	w->sdl2.SDL_SetWindowIcon(w->window, iconSurface);
	w->sdl2.SDL_FreeSurface(iconSurface);
	tex_destroy(icon);
#endif
}

int finalize(struct sdl_window *w) {
	// TODO: rbuf *may* actually need to be guarded with a mutex.
	// In the driver, all access to it happens synchronously from the main
	// thread, at least for now. But theoretically calls to e.g. cr_renderer_restart_interactive()
	// could be coming from an application thread, and that may resize the cr_bitmap rbuf points to.
	int width = (*w->rbuf)->width;
	int height = (*w->rbuf)->height;
	
	w->window = w->sdl2.SDL_CreateWindow("c-ray", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
										width, height,
										SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!w->window) {
		logr(warning, "sdl2 failed to create window: %s\n", w->sdl2.SDL_GetError());
		goto fail_window;
	}

	w->renderer = w->sdl2.SDL_CreateRenderer(w->window, -1, SDL_RENDERER_ACCELERATED);
	if (!w->renderer)
		w->renderer = w->sdl2.SDL_CreateRenderer(w->window, -1, SDL_RENDERER_SOFTWARE);
	if (!w->renderer) {
		logr(warning, "sdl2 failed to create renderer: %s\n", w->sdl2.SDL_GetError());
		goto fail_renderer;
	}

	w->sdl2.SDL_RenderSetLogicalSize(w->renderer, width, height);
	w->sdl2.SDL_SetRenderDrawBlendMode(w->renderer, SDL_BLENDMODE_BLEND);
	// Init pixel texture
	uint32_t format = SDL_PIXELFORMAT_ABGR8888;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	format = SDL_PIXELFORMAT_RGBA8888;
#endif
	w->texture_sdl = w->sdl2.SDL_CreateTexture(w->renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!w->texture_sdl) {
		logr(warning, "sdl2 failed to create texture: %s\n", w->sdl2.SDL_GetError());
		goto fail_texture_sdl;
	}

	w->overlay_sdl = w->sdl2.SDL_CreateTexture(w->renderer, format, SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!w->overlay_sdl) {
		logr(warning, "sdl2 failed to create overlay texture: %s\n", w->sdl2.SDL_GetError());
		goto fail_overlay_sdl;
	}
	// TODO: Is there really no way to use the actual render buffer instead of
	// doing expensive pixel-per-pixel copies in win_update()?
	w->overlay = tex_new(char_p, width, height, 4);
	if (!w->overlay) {
		logr(warning, "sdl2 failed to allocate overlay texture\n");
		goto fail_overlay;
	}
	w->texture = tex_new(char_p, width, height, 4);
	if (!w->texture) {
		logr(warning, "sdl2 failed to allocate texture\n");
		goto fail_texture;
	}

	//And set blend modes for textures too
	w->sdl2.SDL_SetTextureBlendMode(w->texture_sdl, SDL_BLENDMODE_BLEND);
	w->sdl2.SDL_SetTextureBlendMode(w->overlay_sdl, SDL_BLENDMODE_BLEND);

	set_window_icon(w);

	return 0;
fail_texture:
	tex_destroy(w->overlay);
fail_overlay:
	w->sdl2.SDL_DestroyTexture(w->overlay_sdl);
fail_overlay_sdl:
	w->sdl2.SDL_DestroyTexture(w->texture_sdl);
fail_texture_sdl:
	w->sdl2.SDL_DestroyRenderer(w->renderer);
fail_renderer:
	w->sdl2.SDL_DestroyWindow(w->window);
fail_window:
	return 1;
}
// Experimenting with single return + goto pattern in this function
// for no particular reason other than to see if I like it.
struct sdl_window *win_try_init(const struct cr_bitmap **buf) {
	struct sdl_syms syms = try_get_sdl2_syms();
	if (!syms.lib)
		goto fail_syms;

	if (syms.SDL_VideoInit(NULL) < 0) {
		logr(warning, "SDL couldn't initialize: %s\n", syms.SDL_GetError());
		goto fail_sdl_video_init;
	}

	struct sdl_window *w = calloc(1, sizeof(*w));
	if (!w)
		goto fail_calloc;
	w->sdl2 = syms;
	w->rbuf = buf;

	finalize(w);

	return w;
fail_calloc:
	syms.SDL_VideoQuit();
fail_sdl_video_init:
	v_mod_close(syms.lib);
fail_syms:
	return NULL;
}

void win_destroy(struct sdl_window *w) {
	w->sdl2.SDL_DestroyTexture(w->texture_sdl);
	w->sdl2.SDL_DestroyTexture(w->overlay_sdl);
	tex_destroy(w->texture);
	tex_destroy(w->overlay);
	w->sdl2.SDL_DestroyRenderer(w->renderer);
	w->sdl2.SDL_DestroyWindow(w->window);
	w->sdl2.SDL_VideoQuit();
	w->sdl2.SDL_Quit();
	v_mod_close(w->sdl2.lib);
	free(w);
}

static enum input_event check_input(struct sdl_syms *sdl) {
	enum input_event e = ev_none;
	SDL_Event event;
	while (sdl->SDL_PollEvent(&event)) {
		if (e == ev_none && event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			if (event.key.keysym.sym == SDLK_s) {
				printf("\n");
				logr(info, "Aborting render, saving\n");
				e = ev_stop;
			}
			if (event.key.keysym.sym == SDLK_x) {
				printf("\n");
				logr(info, "Aborting render without saving\n");
				e = ev_stop_nosave;
			}
			if (event.key.keysym.sym == SDLK_p) {
				e = ev_pause;
			}
		}
		if (event.type == SDL_QUIT) {
			printf("\n");
			logr(info, "Aborting render without saving\n");
			return ev_stop_nosave;
		}
	}
	return e;
}

static void draw_bar(struct texture *overlay, const struct cr_tile *t) {
	float prc = ((float)t->completed_samples / t->total_samples);
	size_t pixels = (int)((float)t->w * prc);
	struct color c = t->state == cr_tile_rendering ? g_prog_color : g_clear_color;
	for (size_t i = 0; i < pixels; ++i) {
		tex_set_px(overlay, c, t->start_x + i, (t->start_y + (t->h / 5)) - 1);
		tex_set_px(overlay, c, t->start_x + i, (t->start_y + (t->h / 5))    );
		tex_set_px(overlay, c, t->start_x + i, (t->start_y + (t->h / 5)) + 1);
	}
}

static void draw_prog_bars(struct texture *overlay, const struct cr_tile *tiles, size_t tile_count) {
	for (size_t tile = 0; tile < tile_count; ++tile)
		draw_bar(overlay, &tiles[tile]);
}

static void draw_frame(struct texture *buf, struct cr_tile tile, struct color c) {
	int length = tile.w  <= 16 ? 4 : 8;
	length = tile.h <= 16 ? 4 : 8;
	length = length > tile.w ? tile.w : length;
	length = length > tile.h ? tile.h : length;

	for (int i = 1; i < length; ++i) {
		//top left
		tex_set_px(buf, c, tile.start_x + i, tile.start_y + 1);
		tex_set_px(buf, c, tile.start_x + 1, tile.start_y + i);
		
		//top right
		tex_set_px(buf, c, tile.end_x - i, tile.start_y + 1);
		tex_set_px(buf, c, tile.end_x - 1, tile.start_y + i);
		
		//Bottom left
		tex_set_px(buf, c, tile.start_x + i, tile.end_y - 1);
		tex_set_px(buf, c, tile.start_x + 1, tile.end_y - i);
		
		//bottom right
		tex_set_px(buf, c, tile.end_x - i, tile.end_y - 1);
		tex_set_px(buf, c, tile.end_x - 1, tile.end_y - i);
	}
}

static void draw_frames(struct texture *overlay, const struct cr_tile *tiles, size_t tile_count) {
	for (size_t i = 0; i < tile_count; ++i) {
		struct cr_tile tile = tiles[i];
		if (tile.w < 8 || tile.h < 8) return;
		struct color c = tile.state == cr_tile_rendering ? g_frame_color : g_clear_color;
		draw_frame(overlay, tile, c);
	}
}


enum input_event win_update(struct sdl_window *w, const struct cr_tile *tiles, size_t tile_count) {
	if (!w || (!w->window && finalize(w)))
		return ev_none;

	ASSERT(w->internal->precision == char_p);
	ASSERT(t->precision == float_p);
	ASSERT(w->internal->width == t->width);
	ASSERT(w->internal->height == t->height);

	struct texture *t = *((struct texture **)w->rbuf);

	unsigned char *restrict dst = w->texture->data.byte_p;
	float *restrict src = t->data.float_p;

	const int width = w->texture->width;
	for (size_t i = 0; i < tile_count; ++i) {
		for (int y = 0; y < tiles[i].h; ++y) {
			for (int x = 0; x < tiles[i].w; ++x) {
				const int ax = x + tiles[i].start_x;
				const int ay = y + tiles[i].start_y;
				dst[(ax + (ay * width)) * 4 + 0] = (unsigned char)min(linearToSRGB(src[(ax + (ay * width)) * 4 + 0]) * 255.0f, 255.0f);
				dst[(ax + (ay * width)) * 4 + 1] = (unsigned char)min(linearToSRGB(src[(ax + (ay * width)) * 4 + 1]) * 255.0f, 255.0f);
				dst[(ax + (ay * width)) * 4 + 2] = (unsigned char)min(linearToSRGB(src[(ax + (ay * width)) * 4 + 2]) * 255.0f, 255.0f);
				dst[(ax + (ay * width)) * 4 + 3] = 255;
			}
		}
	}
	// Render frames
	// TODO: if (r->prefs.interactive|| r->state.clients) {
	draw_frames(w->overlay, tiles, tile_count);
	draw_prog_bars(w->overlay, tiles, tile_count);
	//Update image data
	if (t)
		w->sdl2.SDL_UpdateTexture(w->texture_sdl, NULL, w->texture->data.byte_p, (int)w->texture->width * 4);
	w->sdl2.SDL_UpdateTexture(w->overlay_sdl, NULL, w->overlay->data.byte_p, (int)w->overlay->width * 4);
	w->sdl2.SDL_RenderCopy(w->renderer, w->texture_sdl, NULL, NULL);
	w->sdl2.SDL_RenderCopy(w->renderer, w->overlay_sdl, NULL, NULL);
	w->sdl2.SDL_RenderPresent(w->renderer);
	return check_input(&w->sdl2);
}
