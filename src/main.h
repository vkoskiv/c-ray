//
//  main.h
//  
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#ifndef ____main__
#define ____main__

#include "includes.h"
#include "filehandler.h"
#include "errorhandler.h"
#include "vector.h"
#include "color.h"
#include "sphere.h"
#include "scene.h"

//These are for multi-platform physical core detection
#ifdef MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

//Some macros
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define invsqrtf(x) (1.0f / sqrtf(x))

#endif /* defined(____main__) */
