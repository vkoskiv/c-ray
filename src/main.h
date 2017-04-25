//
//  main.h
//
//
//  Created by Valtteri Koskivuori on 12/02/15.
//
//

#pragma once

#include "includes.h"
#include "filehandler.h"
#include "errorhandler.h"
#include "vector.h"
#include "color.h"
#include "sphere.h"
#include "renderer.h"
#include "ui.h"

//These are for multi-platform physical core detection
#ifdef MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#elif _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
