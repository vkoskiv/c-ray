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
 *  \file SDL_endian.h
 *
 *  Functions for reading and writing endian-specific values
 */

#ifndef SDL_endian_h_
#define SDL_endian_h_

#include "SDL_stdinc.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
/* As of Clang 11, '_m_prefetchw' is conflicting with the winnt.h's version,
   so we define the needed '_m_prefetch' here as a pseudo-header, until the issue is fixed. */
#ifdef __clang__
#ifndef __PRFCHWINTRIN_H
#define __PRFCHWINTRIN_H
static __inline__ void __attribute__((__always_inline__, __nodebug__))
_m_prefetch(void *__P)
{
  __builtin_prefetch(__P, 0, 3 /* _MM_HINT_T0 */);
}
#endif /* __PRFCHWINTRIN_H */
#endif /* __clang__ */

#include <intrin.h>
#endif

/**
 *  \name The two types of endianness
 */
/* @{ */
#define SDL_LIL_ENDIAN  1234
#define SDL_BIG_ENDIAN  4321
/* @} */

#ifndef SDL_BYTEORDER           /* Not defined in SDL_config.h? */
#ifdef __linux__
#include <endian.h>
#define SDL_BYTEORDER  __BYTE_ORDER
#elif defined(__OpenBSD__) || defined(__DragonFly__)
#include <endian.h>
#define SDL_BYTEORDER  BYTE_ORDER
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/endian.h>
#define SDL_BYTEORDER  BYTE_ORDER
/* predefs from newer gcc and clang versions: */
#elif defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__) && defined(__BYTE_ORDER__)
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define SDL_BYTEORDER   SDL_LIL_ENDIAN
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SDL_BYTEORDER   SDL_BIG_ENDIAN
#else
#error Unsupported endianness
#endif /**/
#else
#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MIPSEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(__powerpc__) || defined(__PPC__) || \
    defined(__sparc__)
#define SDL_BYTEORDER   SDL_BIG_ENDIAN
#else
#define SDL_BYTEORDER   SDL_LIL_ENDIAN
#endif
#endif /* __linux__ */
#endif /* !SDL_BYTEORDER */

#ifndef SDL_FLOATWORDORDER           /* Not defined in SDL_config.h? */
/* predefs from newer gcc versions: */
#if defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__) && defined(__FLOAT_WORD_ORDER__)
#if (__FLOAT_WORD_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define SDL_FLOATWORDORDER   SDL_LIL_ENDIAN
#elif (__FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__)
#define SDL_FLOATWORDORDER   SDL_BIG_ENDIAN
#else
#error Unsupported endianness
#endif /**/
#elif defined(__MAVERICK__)
/* For Maverick, float words are always little-endian. */
#define SDL_FLOATWORDORDER   SDL_LIL_ENDIAN
#elif (defined(__arm__) || defined(__thumb__)) && !defined(__VFP_FP__) && !defined(__ARM_EABI__)
/* For FPA, float words are always big-endian. */
#define SDL_FLOATWORDORDER   SDL_BIG_ENDIAN
#else
/* By default, assume that floats words follow the memory system mode. */
#define SDL_FLOATWORDORDER   SDL_BYTEORDER
#endif /* __FLOAT_WORD_ORDER__ */
#endif /* !SDL_FLOATWORDORDER */


#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_endian_h_ */

/* vi: set ts=4 sw=4 expandtab: */
