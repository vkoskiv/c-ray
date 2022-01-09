//
//  includes.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Global constants
#define MAX_CRAY_VERTEX_COUNT 3
#define PI        3.141592653589793238462643383279502f
#define CRAY_MATERIAL_NAME_SIZE 256
#define CRAY_MESH_FILENAME_LENGTH 500

#define RAY_OFFSET_MULTIPLIER 0.0001f

//FIXME: Should be configurable at runtime
#define SAMPLING_STRATEGY Halton

//Some macros
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrt(x) (1.0f / sqrtf(x))
#if defined(__GNUC__) || defined(__clang__)
#define unlikely(x) __builtin_expect(x, false)
#define likely(x)   __builtin_expect(x, true)
#else
#define unlikely(x) (x)
#define likely(x)   (x)
#endif

//Master include file
#ifdef __linux__
#include <signal.h>
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef WINDOWS
	#include <Windows.h>
	#include <time.h>
#else
	#include <sys/time.h> //for gettimeofday()
#endif
