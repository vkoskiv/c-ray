## Status

[![Build Status](https://semaphoreci.com/api/v1/vkoskiv/c-ray/branches/master/badge.svg)](https://semaphoreci.com/vkoskiv/c-ray)

## Synopsis

C-Ray is a simple raytracer built for studying computer graphics. The implementation is by no means the best, the most efficient nor fully standard, but it's simple, and so is the syntax (hopefully!)

It currently supports:
- Real-time render preview using SDL
- reflections
- lights with radius (soft shadows)
- triangles and spheres
- Multithreading
- Lambertian diffuse shading
- Very crude, hard-coded animations (simple do-while loop)
- Scene definition files (Currently disabled)
- Full support for triangulated OBJ model loading, including matrix transforms to composite a scene.
- k-d trees

Things I'm looking to implement:
- Better material support (Currently only color and reflectivity)
- Procedural textures
- Depth of field
- Antialiasing
- Refractive materials (Glass)

## Dependencies

SDL2 (Optional, comment #define UI_ENABLED in includes.h to disable it)
Standard C99/GNU99 with some standard libraries
All other libraries are included as source

## Installation

macOS:
1. Install SDL2
2. Open the .xcodeproj file in Xcode
3. Edit scheme by clicking 'C-Ray' in top right, make sure 'Use custom working directory' is ticked and set it to the 'output' directory of this project.
4. Build&Run with CMD+R

Linux:
1. Install SDL2 (See installing SDL below)
2. Run 'make'
3. Suggest a fix to my makefile because it didn't link SDL2 on your platform.
4. Run binary in ./bin/

Windows:
1. Open the VS project in CRayWindows
2. CTRL+F5 to run without debugging
3. VS places a binary under CRayWindows/bin/

## Installing SDL

On macOS, download the SDL2 runtime framework from https://www.libsdl.org/download-2.0.php and place in /Library/Frameworks/
If you don't have root access, place under ~/Library/Frameworks

On Windows, Visual Studio should include SDL automatically

On Linux, run 'sudo apt install libsdl2-dev'

## Tests

You really thought I wrote tests...?

## Credits

3rd party libraries included in this project

OBJ Loader library: http://www.kixor.net/dev/objloader/

lodePNG PNG compression library: http://lodev.org/lodepng/

SDL2: https://www.libsdl.org/index.php

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

