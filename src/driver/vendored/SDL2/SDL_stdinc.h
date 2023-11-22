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
 *  \file SDL_stdinc.h
 *
 *  This is a general header that includes C language support.
 */

#ifndef SDL_stdinc_h_
#define SDL_stdinc_h_

// #include "SDL_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <stdio.h>
# include <stdlib.h>
# include <stddef.h>
# include <stdarg.h>
#ifdef HAVE_STRING_H
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_WCHAR_H
# include <wchar.h>
#endif
# include <stdint.h>
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_MATH_H
# if defined(_MSC_VER)
/* Defining _USE_MATH_DEFINES is required to get M_PI to be defined on
   Visual Studio.  See http://msdn.microsoft.com/en-us/library/4hwaceh6.aspx
   for more information.
*/
#  ifndef _USE_MATH_DEFINES
#    define _USE_MATH_DEFINES
#  endif
# endif
# include <math.h>
#endif
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#if defined(HAVE_ALLOCA) && !defined(alloca)
# if defined(HAVE_ALLOCA_H)
#  include <alloca.h>
# elif defined(__GNUC__)
#  define alloca __builtin_alloca
# elif defined(_MSC_VER)
#  include <malloc.h>
#  define alloca _alloca
# elif defined(__WATCOMC__)
#  include <malloc.h>
# elif defined(__BORLANDC__)
#  include <malloc.h>
# elif defined(__DMC__)
#  include <stdlib.h>
# elif defined(__AIX__)
#pragma alloca
# elif defined(__MRC__)
void *alloca(unsigned);
# else
char *alloca();
# endif
#endif

#ifdef SIZE_MAX
# define SDL_SIZE_MAX SIZE_MAX
#else
# define SDL_SIZE_MAX ((size_t) -1)
#endif

/**
 * Check if the compiler supports a given builtin.
 * Supported by virtually all clang versions and recent gcc. Use this
 * instead of checking the clang version if possible.
 */
#ifdef __has_builtin
#define _SDL_HAS_BUILTIN(x) __has_builtin(x)
#else
#define _SDL_HAS_BUILTIN(x) 0
#endif

/**
 *  The number of elements in an array.
 */
#define SDL_arraysize(array)    (sizeof(array)/sizeof(array[0]))
#define SDL_TABLESIZE(table)    SDL_arraysize(table)

/**
 *  Macro useful for building other macros with strings in them
 *
 *  e.g. #define LOG_ERROR(X) OutputDebugString(SDL_STRINGIFY_ARG(__FUNCTION__) ": " X "\n")
 */
#define SDL_STRINGIFY_ARG(arg)  #arg

/**
 *  \name Cast operators
 *
 *  Use proper C++ casts when compiled as C++ to be compatible with the option
 *  -Wold-style-cast of GCC (and -Werror=old-style-cast in GCC 4.2 and above).
 */
/* @{ */
#ifdef __cplusplus
#define SDL_reinterpret_cast(type, expression) reinterpret_cast<type>(expression)
#define SDL_static_cast(type, expression) static_cast<type>(expression)
#define SDL_const_cast(type, expression) const_cast<type>(expression)
#else
#define SDL_reinterpret_cast(type, expression) ((type)(expression))
#define SDL_static_cast(type, expression) ((type)(expression))
#define SDL_const_cast(type, expression) ((type)(expression))
#endif
/* @} *//* Cast operators */

/* Define a four character code as a Uint32 */
#define SDL_FOURCC(A, B, C, D) \
    ((SDL_static_cast(Uint32, SDL_static_cast(Uint8, (A))) << 0) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (B))) << 8) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (C))) << 16) | \
     (SDL_static_cast(Uint32, SDL_static_cast(Uint8, (D))) << 24))

/**
 *  \name Basic data types
 */
/* @{ */

#ifdef __CC_ARM
/* ARM's compiler throws warnings if we use an enum: like "SDL_bool x = a < b;" */
#define SDL_FALSE 0
#define SDL_TRUE 1
typedef int SDL_bool;
#else
typedef enum
{
    SDL_FALSE = 0,
    SDL_TRUE = 1
} SDL_bool;
#endif

/**
 * \brief A signed 8-bit integer type.
 */
#define SDL_MAX_SINT8   ((Sint8)0x7F)           /* 127 */
#define SDL_MIN_SINT8   ((Sint8)(~0x7F))        /* -128 */
typedef int8_t Sint8;
/**
 * \brief An unsigned 8-bit integer type.
 */
#define SDL_MAX_UINT8   ((Uint8)0xFF)           /* 255 */
#define SDL_MIN_UINT8   ((Uint8)0x00)           /* 0 */
typedef uint8_t Uint8;
/**
 * \brief A signed 16-bit integer type.
 */
#define SDL_MAX_SINT16  ((Sint16)0x7FFF)        /* 32767 */
#define SDL_MIN_SINT16  ((Sint16)(~0x7FFF))     /* -32768 */
typedef int16_t Sint16;
/**
 * \brief An unsigned 16-bit integer type.
 */
#define SDL_MAX_UINT16  ((Uint16)0xFFFF)        /* 65535 */
#define SDL_MIN_UINT16  ((Uint16)0x0000)        /* 0 */
typedef uint16_t Uint16;
/**
 * \brief A signed 32-bit integer type.
 */
#define SDL_MAX_SINT32  ((Sint32)0x7FFFFFFF)    /* 2147483647 */
#define SDL_MIN_SINT32  ((Sint32)(~0x7FFFFFFF)) /* -2147483648 */
typedef int32_t Sint32;
/**
 * \brief An unsigned 32-bit integer type.
 */
#define SDL_MAX_UINT32  ((Uint32)0xFFFFFFFFu)   /* 4294967295 */
#define SDL_MIN_UINT32  ((Uint32)0x00000000)    /* 0 */
typedef uint32_t Uint32;

/**
 * \brief A signed 64-bit integer type.
 */
#define SDL_MAX_SINT64  ((Sint64)0x7FFFFFFFFFFFFFFFll)      /* 9223372036854775807 */
#define SDL_MIN_SINT64  ((Sint64)(~0x7FFFFFFFFFFFFFFFll))   /* -9223372036854775808 */
typedef int64_t Sint64;
/**
 * \brief An unsigned 64-bit integer type.
 */
#define SDL_MAX_UINT64  ((Uint64)0xFFFFFFFFFFFFFFFFull)     /* 18446744073709551615 */
#define SDL_MIN_UINT64  ((Uint64)(0x0000000000000000ull))   /* 0 */
typedef uint64_t Uint64;

/* @} *//* Basic data types */

/**
 *  \name Floating-point constants
 */
/* @{ */

#ifdef FLT_EPSILON
#define SDL_FLT_EPSILON FLT_EPSILON
#else
#define SDL_FLT_EPSILON 1.1920928955078125e-07F /* 0x0.000002p0 */
#endif

/* @} *//* Floating-point constants */

/* Make sure we have macros for printing width-based integers.
 * <stdint.h> should define these but this is not true all platforms.
 * (for example win32) */
#ifndef SDL_PRIs64
#ifdef PRIs64
#define SDL_PRIs64 PRIs64
#elif defined(__WIN32__) || defined(__GDK__)
#define SDL_PRIs64 "I64d"
#elif defined(__LINUX__) && defined(__LP64__)
#define SDL_PRIs64 "ld"
#else
#define SDL_PRIs64 "lld"
#endif
#endif
#ifndef SDL_PRIu64
#ifdef PRIu64
#define SDL_PRIu64 PRIu64
#elif defined(__WIN32__) || defined(__GDK__)
#define SDL_PRIu64 "I64u"
#elif defined(__LINUX__) && defined(__LP64__)
#define SDL_PRIu64 "lu"
#else
#define SDL_PRIu64 "llu"
#endif
#endif
#ifndef SDL_PRIx64
#ifdef PRIx64
#define SDL_PRIx64 PRIx64
#elif defined(__WIN32__) || defined(__GDK__)
#define SDL_PRIx64 "I64x"
#elif defined(__LINUX__) && defined(__LP64__)
#define SDL_PRIx64 "lx"
#else
#define SDL_PRIx64 "llx"
#endif
#endif
#ifndef SDL_PRIX64
#ifdef PRIX64
#define SDL_PRIX64 PRIX64
#elif defined(__WIN32__) || defined(__GDK__)
#define SDL_PRIX64 "I64X"
#elif defined(__LINUX__) && defined(__LP64__)
#define SDL_PRIX64 "lX"
#else
#define SDL_PRIX64 "llX"
#endif
#endif
#ifndef SDL_PRIs32
#ifdef PRId32
#define SDL_PRIs32 PRId32
#else
#define SDL_PRIs32 "d"
#endif
#endif
#ifndef SDL_PRIu32
#ifdef PRIu32
#define SDL_PRIu32 PRIu32
#else
#define SDL_PRIu32 "u"
#endif
#endif
#ifndef SDL_PRIx32
#ifdef PRIx32
#define SDL_PRIx32 PRIx32
#else
#define SDL_PRIx32 "x"
#endif
#endif
#ifndef SDL_PRIX32
#ifdef PRIX32
#define SDL_PRIX32 PRIX32
#else
#define SDL_PRIX32 "X"
#endif
#endif

/* Annotations to help code analysis tools */
#ifdef SDL_DISABLE_ANALYZE_MACROS
#define SDL_IN_BYTECAP(x)
#define SDL_INOUT_Z_CAP(x)
#define SDL_OUT_Z_CAP(x)
#define SDL_OUT_CAP(x)
#define SDL_OUT_BYTECAP(x)
#define SDL_OUT_Z_BYTECAP(x)
#define SDL_PRINTF_FORMAT_STRING
#define SDL_SCANF_FORMAT_STRING
#define SDL_PRINTF_VARARG_FUNC( fmtargnumber )
#define SDL_SCANF_VARARG_FUNC( fmtargnumber )
#else
#if defined(_MSC_VER) && (_MSC_VER >= 1600) /* VS 2010 and above */
#include <sal.h>

#define SDL_IN_BYTECAP(x) _In_bytecount_(x)
#define SDL_INOUT_Z_CAP(x) _Inout_z_cap_(x)
#define SDL_OUT_Z_CAP(x) _Out_z_cap_(x)
#define SDL_OUT_CAP(x) _Out_cap_(x)
#define SDL_OUT_BYTECAP(x) _Out_bytecap_(x)
#define SDL_OUT_Z_BYTECAP(x) _Out_z_bytecap_(x)

#define SDL_PRINTF_FORMAT_STRING _Printf_format_string_
#define SDL_SCANF_FORMAT_STRING _Scanf_format_string_impl_
#else
#define SDL_IN_BYTECAP(x)
#define SDL_INOUT_Z_CAP(x)
#define SDL_OUT_Z_CAP(x)
#define SDL_OUT_CAP(x)
#define SDL_OUT_BYTECAP(x)
#define SDL_OUT_Z_BYTECAP(x)
#define SDL_PRINTF_FORMAT_STRING
#define SDL_SCANF_FORMAT_STRING
#endif
#if defined(__GNUC__)
#define SDL_PRINTF_VARARG_FUNC( fmtargnumber ) __attribute__ (( format( __printf__, fmtargnumber, fmtargnumber+1 )))
#define SDL_SCANF_VARARG_FUNC( fmtargnumber ) __attribute__ (( format( __scanf__, fmtargnumber, fmtargnumber+1 )))
#else
#define SDL_PRINTF_VARARG_FUNC( fmtargnumber )
#define SDL_SCANF_VARARG_FUNC( fmtargnumber )
#endif
#endif /* SDL_DISABLE_ANALYZE_MACROS */

#ifndef SDL_COMPILE_TIME_ASSERT
#if defined(__cplusplus)
#if (__cplusplus >= 201103L)
#define SDL_COMPILE_TIME_ASSERT(name, x)  static_assert(x, #x)
#endif
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SDL_COMPILE_TIME_ASSERT(name, x) _Static_assert(x, #x)
#endif
#endif /* !SDL_COMPILE_TIME_ASSERT */

#ifndef SDL_COMPILE_TIME_ASSERT
/* universal, but may trigger -Wunused-local-typedefs */
#define SDL_COMPILE_TIME_ASSERT(name, x)               \
       typedef int SDL_compile_time_assert_ ## name[(x) * 2 - 1]
#endif

/** \cond */
#ifndef DOXYGEN_SHOULD_IGNORE_THIS
SDL_COMPILE_TIME_ASSERT(uint8, sizeof(Uint8) == 1);
SDL_COMPILE_TIME_ASSERT(sint8, sizeof(Sint8) == 1);
SDL_COMPILE_TIME_ASSERT(uint16, sizeof(Uint16) == 2);
SDL_COMPILE_TIME_ASSERT(sint16, sizeof(Sint16) == 2);
SDL_COMPILE_TIME_ASSERT(uint32, sizeof(Uint32) == 4);
SDL_COMPILE_TIME_ASSERT(sint32, sizeof(Sint32) == 4);
SDL_COMPILE_TIME_ASSERT(uint64, sizeof(Uint64) == 8);
SDL_COMPILE_TIME_ASSERT(sint64, sizeof(Sint64) == 8);
#endif /* DOXYGEN_SHOULD_IGNORE_THIS */
/** \endcond */

/* Check to make sure enums are the size of ints, for structure packing.
   For both Watcom C/C++ and Borland C/C++ the compiler option that makes
   enums having the size of an int must be enabled.
   This is "-b" for Borland C/C++ and "-ei" for Watcom C/C++ (v11).
*/

/** \cond */
#ifndef DOXYGEN_SHOULD_IGNORE_THIS
#if !defined(__ANDROID__) && !defined(__VITA__) && !defined(__3DS__)
   /* TODO: include/SDL_stdinc.h:174: error: size of array 'SDL_dummy_enum' is negative */
typedef enum
{
    DUMMY_ENUM_VALUE
} SDL_DUMMY_ENUM;

SDL_COMPILE_TIME_ASSERT(enum, sizeof(SDL_DUMMY_ENUM) == sizeof(int));
#endif
#endif /* DOXYGEN_SHOULD_IGNORE_THIS */
/** \endcond */

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_ALLOCA
#define SDL_stack_alloc(type, count)    (type*)alloca(sizeof(type)*(count))
#define SDL_stack_free(data)
#else
#define SDL_stack_alloc(type, count)    (type*)SDL_malloc(sizeof(type)*(count))
#define SDL_stack_free(data)            SDL_free(data)
#endif

#ifndef HAVE_M_PI
#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288   /**< pi */
#endif
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* SDL_stdinc_h_ */

/* vi: set ts=4 sw=4 expandtab: */
