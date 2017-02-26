//
//  includes.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#ifndef C_Ray_includes_h
#define C_Ray_includes_h

//Global constants
#define MAX_CRAY_VERTEX_COUNT 3
#define PIOVER180 0.017453292519943295769236907684886
#define PI        3.141592653589793238462643383279502

//Some macros
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

//Master include file
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <signal.h>
#include <string.h>
#ifdef WINDOWS
	#include <Windows.h>
	#include "SDL.h"
#else
	#include <pthread.h>
	#include <SDL2/SDL.h>
#endif
#include "lodepng.h"
#include "obj_parser.h"

#endif
