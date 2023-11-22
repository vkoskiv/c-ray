/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2023 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/**
 *  \file SDL_surface.h
 *
 *  Header file for ::SDL_Surface definition and management functions.
 */

#ifndef SDL_surface_h_
#define SDL_surface_h_

#include "SDL_stdinc.h"
#include "SDL_pixels.h"
#include "SDL_rect.h"
#include "SDL_blendmode.h"
#include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \name Surface flags
 *
 *  These are the currently supported flags for the ::SDL_Surface.
 *
 *  \internal
 *  Used internally (read-only).
 */
/* @{ */
#define SDL_SWSURFACE       0           /**< Just here for compatibility */
#define SDL_PREALLOC        0x00000001  /**< Surface uses preallocated memory */
#define SDL_RLEACCEL        0x00000002  /**< Surface is RLE encoded */
#define SDL_DONTFREE        0x00000004  /**< Surface is referenced internally */
#define SDL_SIMD_ALIGNED    0x00000008  /**< Surface uses aligned memory */
/* @} *//* Surface flags */

/**
 *  Evaluates to true if the surface needs to be locked before access.
 */
#define SDL_MUSTLOCK(S) (((S)->flags & SDL_RLEACCEL) != 0)

typedef struct SDL_BlitMap SDL_BlitMap;  /* this is an opaque type. */

/**
 * \brief A collection of pixels used in software blitting.
 *
 * \note  This structure should be treated as read-only, except for \c pixels,
 *        which, if not NULL, contains the raw pixel data for the surface.
 */
typedef struct SDL_Surface
{
    Uint32 flags;               /**< Read-only */
    SDL_PixelFormat *format;    /**< Read-only */
    int w, h;                   /**< Read-only */
    int pitch;                  /**< Read-only */
    void *pixels;               /**< Read-write */

    /** Application data associated with the surface */
    void *userdata;             /**< Read-write */

    /** information needed for surfaces requiring locks */
    int locked;                 /**< Read-only */

    /** list of BlitMap that hold a reference to this surface */
    void *list_blitmap;         /**< Private */

    /** clipping information */
    SDL_Rect clip_rect;         /**< Read-only */

    /** info for fast blit mapping to other surfaces */
    SDL_BlitMap *map;           /**< Private */

    /** Reference count -- used when freeing surface */
    int refcount;               /**< Read-mostly */
} SDL_Surface;

/**
 * \brief The type of function used for surface blitting functions.
 */
typedef int (SDLCALL *SDL_blit) (struct SDL_Surface * src, SDL_Rect * srcrect,
                                 struct SDL_Surface * dst, SDL_Rect * dstrect);

/**
 * \brief The formula used for converting between YUV and RGB
 */
typedef enum
{
    SDL_YUV_CONVERSION_JPEG,        /**< Full range JPEG */
    SDL_YUV_CONVERSION_BT601,       /**< BT.601 (the default) */
    SDL_YUV_CONVERSION_BT709,       /**< BT.709 */
    SDL_YUV_CONVERSION_AUTOMATIC    /**< BT.601 for SD content, BT.709 for HD content */
} SDL_YUV_CONVERSION_MODE;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_surface_h_ */

/* vi: set ts=4 sw=4 expandtab: */
