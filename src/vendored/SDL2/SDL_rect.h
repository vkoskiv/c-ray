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
 *  \file SDL_rect.h
 *
 *  Header file for SDL_rect definition and management functions.
 */

#ifndef SDL_rect_h_
#define SDL_rect_h_

#include "SDL_stdinc.h"
// #include "SDL_error.h"
// #include "SDL_pixels.h"
// #include "SDL_rwops.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * The structure that defines a point (integer)
 *
 * \sa SDL_EnclosePoints
 * \sa SDL_PointInRect
 */
typedef struct SDL_Point
{
    int x;
    int y;
} SDL_Point;

/**
 * The structure that defines a point (floating point)
 *
 * \sa SDL_EncloseFPoints
 * \sa SDL_PointInFRect
 */
typedef struct SDL_FPoint
{
    float x;
    float y;
} SDL_FPoint;


/**
 * A rectangle, with the origin at the upper left (integer).
 *
 * \sa SDL_RectEmpty
 * \sa SDL_RectEquals
 * \sa SDL_HasIntersection
 * \sa SDL_IntersectRect
 * \sa SDL_IntersectRectAndLine
 * \sa SDL_UnionRect
 * \sa SDL_EnclosePoints
 */
typedef struct SDL_Rect
{
    int x, y;
    int w, h;
} SDL_Rect;


/**
 * A rectangle, with the origin at the upper left (floating point).
 *
 * \sa SDL_FRectEmpty
 * \sa SDL_FRectEquals
 * \sa SDL_FRectEqualsEpsilon
 * \sa SDL_HasIntersectionF
 * \sa SDL_IntersectFRect
 * \sa SDL_IntersectFRectAndLine
 * \sa SDL_UnionFRect
 * \sa SDL_EncloseFPoints
 * \sa SDL_PointInFRect
 */
typedef struct SDL_FRect
{
    float x;
    float y;
    float w;
    float h;
} SDL_FRect;


/**
 * Returns true if point resides inside a rectangle.
 */
SDL_FORCE_INLINE SDL_bool SDL_PointInRect(const SDL_Point *p, const SDL_Rect *r)
{
    return ( (p->x >= r->x) && (p->x < (r->x + r->w)) &&
             (p->y >= r->y) && (p->y < (r->y + r->h)) ) ? SDL_TRUE : SDL_FALSE;
}

/**
 * Returns true if the rectangle has no area.
 */
SDL_FORCE_INLINE SDL_bool SDL_RectEmpty(const SDL_Rect *r)
{
    return ((!r) || (r->w <= 0) || (r->h <= 0)) ? SDL_TRUE : SDL_FALSE;
}

/**
 * Returns true if the two rectangles are equal.
 */
SDL_FORCE_INLINE SDL_bool SDL_RectEquals(const SDL_Rect *a, const SDL_Rect *b)
{
    return (a && b && (a->x == b->x) && (a->y == b->y) &&
            (a->w == b->w) && (a->h == b->h)) ? SDL_TRUE : SDL_FALSE;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_rect_h_ */

/* vi: set ts=4 sw=4 expandtab: */
