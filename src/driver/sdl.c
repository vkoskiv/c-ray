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
#include "../common/texture.h"
#include "../common/color.h"
#include "../common/logging.h"
#include "../common/assert.h"
#include "../common/logo.h"
#include "../common/loaders/textureloader.h"
#include "../common/platform/thread.h"
#include "../common/platform/dyn.h"
#include "../common/vendored/cJSON.h"
#include "signal.h"
#include <c-ray/c-ray.h>

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
		"libSDL2-2.0.so",
		"libSDL2-2.0.0.dylib",
		"SDL2.dll"
	};
	void *lib = NULL;
	for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
		if ((lib = dyn_load(candidates[i]))) return lib;
	}

	logr(info, "Couldn't find SDL library (%s), tried the following names: ", dyn_error());
	for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
		logr(plain, "\"%s\" ", candidates[i]);
	}
	logr(plain, "\n");

	return NULL;
}

static struct sdl_syms *try_get_sdl2_syms(void) {
	void *sdl2 = try_find_sdl2_lib();
	if (!sdl2) return NULL;
	struct sdl_syms *syms = calloc(1, sizeof(*syms));
	*syms = (struct sdl_syms){
		.lib = sdl2,
		.SDL_VideoInit              = dyn_sym(sdl2, "SDL_VideoInit"),
		.SDL_VideoQuit              = dyn_sym(sdl2, "SDL_VideoQuit"),
		.SDL_Quit                   = dyn_sym(sdl2, "SDL_Quit"),
		.SDL_GetError               = dyn_sym(sdl2, "SDL_GetError"),
		.SDL_SetWindowIcon          = dyn_sym(sdl2, "SDL_SetWindowIcon"),
		.SDL_FreeSurface            = dyn_sym(sdl2, "SDL_FreeSurface"),
		.SDL_CreateRGBSurfaceFrom   = dyn_sym(sdl2, "SDL_CreateRGBSurfaceFrom"),
		.SDL_CreateWindow           = dyn_sym(sdl2, "SDL_CreateWindow"),
		.SDL_CreateRenderer         = dyn_sym(sdl2, "SDL_CreateRenderer"),
		.SDL_CreateTexture          = dyn_sym(sdl2, "SDL_CreateTexture"),
		.SDL_DestroyTexture         = dyn_sym(sdl2, "SDL_DestroyTexture"),
		.SDL_DestroyRenderer        = dyn_sym(sdl2, "SDL_DestroyRenderer"),
		.SDL_DestroyWindow          = dyn_sym(sdl2, "SDL_DestroyWindow"),
		.SDL_RenderPresent          = dyn_sym(sdl2, "SDL_RenderPresent"),
		.SDL_RenderSetLogicalSize   = dyn_sym(sdl2, "SDL_RenderSetLogicalSize"),
		.SDL_SetRenderDrawBlendMode = dyn_sym(sdl2, "SDL_SetRenderDrawBlendMode"),
		.SDL_SetTextureBlendMode    = dyn_sym(sdl2, "SDL_SetTextureBlendMode"),
		.SDL_RenderSetScale         = dyn_sym(sdl2, "SDL_RenderSetScale"),
		.SDL_PollEvent              = dyn_sym(sdl2, "SDL_PollEvent"),
		.SDL_UpdateTexture          = dyn_sym(sdl2, "SDL_UpdateTexture"),
		.SDL_RenderCopy             = dyn_sym(sdl2, "SDL_RenderCopy")
	};
	for (size_t i = 0; i < (sizeof(struct sdl_syms) / sizeof(void *)); ++i) {
		if (!((void **)syms)[i]) {
			logr(warning, "sdl_syms[%zu] is NULL\n", i);
			free(syms);
			return NULL;
		}
	}
	return syms;
}

struct sdl_window {
	struct sdl_syms *sym;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Texture *overlay_sdl;
	struct texture *internal;
	struct texture *overlay;
	bool isBorderless;
	bool isFullScreen;
	float windowScale;
	
	unsigned width;
	unsigned height;
};


static void setWindowIcon(struct sdl_window *w) {
#ifndef NO_LOGO
	struct texture *icon = load_texture("logo.h", (file_data){ .items = logo_png_data, .count = logo_png_data_len });
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
	SDL_Surface *iconSurface = w->sym->SDL_CreateRGBSurfaceFrom(icon->data.byte_p,
														(int)icon->width,
														(int)icon->height,
														(int)icon->channels * 8,
														(int)(icon->channels * icon->width),
														rmask, gmask, bmask, amask);
	w->sym->SDL_SetWindowIcon(w->window, iconSurface);
	w->sym->SDL_FreeSurface(iconSurface);
	destroyTexture(icon);
#endif
}

struct sdl_window *win_try_init(struct sdl_prefs *prefs, int width, int height) {
	if (!prefs->enabled) return NULL;
	struct sdl_syms *syms = try_get_sdl2_syms();
	if (!syms) return NULL;

	struct sdl_window *w = calloc(1, sizeof(*w));
	w->sym = syms;

	w->isFullScreen = prefs->fullscreen;
	w->isBorderless = prefs->borderless;
	w->windowScale = prefs->scale;
	w->width = width;
	w->height = height;
	
	//Initialize SDL
	if (w->sym->SDL_VideoInit(NULL) < 0) {
		logr(warning, "SDL couldn't initialize, error: \"%s\"\n", w->sym->SDL_GetError());
		win_destroy(w);
		return NULL;
	}
	//Init window
	SDL_WindowFlags flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
	if (prefs->fullscreen) flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	if (prefs->borderless) flags |= SDL_WINDOW_BORDERLESS;
	flags |= SDL_WINDOW_RESIZABLE;

	w->window = w->sym->SDL_CreateWindow("c-ray © vkoskiv 2015-2023",
										 SDL_WINDOWPOS_UNDEFINED,
										 SDL_WINDOWPOS_UNDEFINED,
								 width * prefs->scale,
								 height * prefs->scale,
								 flags);
	if (w->window == NULL) {
		logr(warning, "Window couldn't be created, error: \"%s\"\n", w->sym->SDL_GetError());
		win_destroy(w);
		return NULL;
	}
	//Init renderer
	w->renderer = w->sym->SDL_CreateRenderer(w->window, -1, SDL_RENDERER_ACCELERATED);
	if (!w->renderer) w->renderer = w->sym->SDL_CreateRenderer(w->window, -1, SDL_RENDERER_SOFTWARE);
	if (!w->renderer) {
		logr(warning, "Renderer couldn't be created, error: \"%s\"\n", w->sym->SDL_GetError());
		win_destroy(w);
		return NULL;
	}
	
	w->sym->SDL_RenderSetLogicalSize(w->renderer, w->width, w->height);
	//And set blend modes
	w->sym->SDL_SetRenderDrawBlendMode(w->renderer, SDL_BLENDMODE_BLEND);
	
	w->sym->SDL_RenderSetScale(w->renderer, w->windowScale, w->windowScale);
	//Init pixel texture
	uint32_t format = SDL_PIXELFORMAT_ABGR8888;
#if SDL_BYTEORDER == BIG_ENDIAN
	format = SDL_PIXELFORMAT_RGBA8888;
#endif
	w->texture = w->sym->SDL_CreateTexture(w->renderer, format, SDL_TEXTUREACCESS_STREAMING, w->width, w->height);
	if (w->texture == NULL) {
		logr(warning, "Texture couldn't be created, error: \"%s\"\n", w->sym->SDL_GetError());
		win_destroy(w);
		return NULL;
	}
	//Init overlay texture (for UI info)

	w->overlay_sdl = w->sym->SDL_CreateTexture(w->renderer, format, SDL_TEXTUREACCESS_STREAMING, w->width, w->height);
	if (w->overlay_sdl == NULL) {
		logr(warning, "Overlay texture couldn't be created, error: \"%s\"\n", w->sym->SDL_GetError());
		win_destroy(w);
		return NULL;
	}
	w->overlay = newTexture(char_p, width, height, 4);
	w->internal = newTexture(char_p, width, height, 4);
	
	//And set blend modes for textures too
	w->sym->SDL_SetTextureBlendMode(w->texture, SDL_BLENDMODE_BLEND);
	w->sym->SDL_SetTextureBlendMode(w->overlay_sdl, SDL_BLENDMODE_BLEND);
	
	setWindowIcon(w);
	
	return w;
}

void win_destroy(struct sdl_window *w) {
	if (!w) return;
	if (w->sym) {
		if (w->texture) {
			w->sym->SDL_DestroyTexture(w->texture);
			w->texture = NULL;
		}
		if (w->overlay_sdl) {
			w->sym->SDL_DestroyTexture(w->overlay_sdl);
			w->texture = NULL;
		}
		destroyTexture(w->internal);
		destroyTexture(w->overlay);
		if (w->renderer) {
			w->sym->SDL_DestroyRenderer(w->renderer);
			w->renderer = NULL;
		}
		if (w->window) {
			w->sym->SDL_DestroyWindow(w->window);
			w->window = NULL;
		}
		w->sym->SDL_VideoQuit();
		w->sym->SDL_Quit();
		dyn_close(w->sym->lib);
		free(w->sym);
	}
	free(w);
	w = NULL;
}

static struct input_state win_check_keyboard(struct sdl_window *sdl) {
	struct input_state cmd = { .should_save = true };
	if (!sdl) return cmd;
	SDL_Event event;
	while (sdl->sym->SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
			if (event.key.keysym.sym == SDLK_s) {
				printf("\n");
				logr(info, "Aborting render, saving\n");
				cmd.stop_render = true;
			}
			if (event.key.keysym.sym == SDLK_x) {
				printf("\n");
				logr(info, "Aborting render without saving\n");
				cmd.stop_render = true;
				cmd.should_save = false;
			}
			if (event.key.keysym.sym == SDLK_p) {
				cmd.pause_render = true;
			}
		}
		if (event.type == SDL_QUIT) {
			printf("\n");
			logr(info, "Aborting render without saving\n");
			cmd.stop_render = true;
			cmd.should_save = false;
		}
	}
	return cmd;
}

static void draw_bar(struct texture *overlay, const struct cr_tile *t) {
	float prc = ((float)t->completed_samples / t->total_samples);
	size_t pixels = (int)((float)t->w * prc);
	struct color c = t->state == cr_tile_rendering ? g_prog_color : g_clear_color;
	for (size_t i = 0; i < pixels; ++i) {
		setPixel(overlay, c, t->start_x + i, (t->start_y + (t->h / 5)) - 1);
		setPixel(overlay, c, t->start_x + i, (t->start_y + (t->h / 5))    );
		setPixel(overlay, c, t->start_x + i, (t->start_y + (t->h / 5)) + 1);
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
		setPixel(buf, c, tile.start_x + i, tile.start_y + 1);
		setPixel(buf, c, tile.start_x + 1, tile.start_y + i);
		
		//top right
		setPixel(buf, c, tile.end_x - i, tile.start_y + 1);
		setPixel(buf, c, tile.end_x - 1, tile.start_y + i);
		
		//Bottom left
		setPixel(buf, c, tile.start_x + i, tile.end_y - 1);
		setPixel(buf, c, tile.start_x + 1, tile.end_y - i);
		
		//bottom right
		setPixel(buf, c, tile.end_x - i, tile.end_y - 1);
		setPixel(buf, c, tile.end_x - 1, tile.end_y - i);
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


struct input_state win_update(struct sdl_window *w, const struct cr_tile *tiles, size_t tile_count, const struct texture *t) {
	if (!w) return (struct input_state){ 0 };
	// Copy regions
	ASSERT(w->internal->precision == char_p);
	ASSERT(t->precision == float_p);
	ASSERT(w->internal->width == t->width);
	ASSERT(w->internal->height == t->height);

	unsigned char *restrict dst = w->internal->data.byte_p;
	float *restrict src = t->data.float_p;
	
	const int width = w->internal->width;
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
	//Render frames
	// TODO: if (r->prefs.iterative || r->state.clients) {
	draw_frames(w->overlay, tiles, tile_count);
	draw_prog_bars(w->overlay, tiles, tile_count);
	//Update image data
	if (t) w->sym->SDL_UpdateTexture(w->texture, NULL, w->internal->data.byte_p, (int)w->width * 4);
	w->sym->SDL_UpdateTexture(w->overlay_sdl, NULL, w->overlay->data.byte_p, (int)w->width * 4);
	w->sym->SDL_RenderCopy(w->renderer, w->texture, NULL, NULL);
	w->sym->SDL_RenderCopy(w->renderer, w->overlay_sdl, NULL, NULL);
	w->sym->SDL_RenderPresent(w->renderer);
	return win_check_keyboard(w);
}

struct sdl_prefs sdl_parse(const cJSON *data) {
	struct sdl_prefs prefs = { 0 };
	prefs.scale = 1.0f;
	prefs.enabled = true;
	if (!data) return prefs;

	const cJSON *enabled = cJSON_GetObjectItem(data, "enabled");
	if (cJSON_IsBool(enabled))
		prefs.enabled = cJSON_IsTrue(enabled);

	const cJSON *isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (cJSON_IsBool(isFullscreen))
		prefs.fullscreen = cJSON_IsTrue(isFullscreen);

	const cJSON *isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (cJSON_IsBool(isBorderless))
		prefs.borderless = cJSON_IsTrue(isBorderless);

	const cJSON *windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (cJSON_IsNumber(windowScale) && windowScale->valuedouble >= 0)
		prefs.scale = windowScale->valuedouble;
	return prefs;
}
