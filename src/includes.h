//
//  includes.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Global constants
#define PI        3.141592653589793238462643383279502f
#define CRAY_MATERIAL_NAME_SIZE 256
#define CRAY_MESH_FILENAME_LENGTH 500

#define RAY_OFFSET_MULTIPLIER 0.0001f

//FIXME: Should be configurable at runtime
#define SAMPLING_STRATEGY Halton

#ifdef __GNUC__
#define CR_UNUSED __attribute__((unused))
#else
#define CR_UNUSED
#endif

//Some macros
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))
#if defined(__GNUC__) || defined(__clang__)
#define unlikely(x) __builtin_expect(x, false)
#define likely(x)   __builtin_expect(x, true)
#else
#define unlikely(x) (x)
#define likely(x)   (x)
#endif

//Master include file
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef unsigned char byte;

#ifdef WINDOWS
	#include <Windows.h>
	#include <time.h>
#else
	#include <sys/time.h> //for gettimeofday()
#endif
