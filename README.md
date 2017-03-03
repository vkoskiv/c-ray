## Synopsis

C-Ray is a simple raytracer built for studying computer graphics. The implementation is by no means the best, the most efficient nor fully standard, but it's simple, and so is the syntax (hopefully!)

It currently supports:
- reflections
- lights with radius (soft shadows)
- triangles and spheres
- Multithreading
- Lambertian diffuse shading
- Very crude, hard-coded animations (simple do-while loop)
- Scene definition files (Currently disabled)

Things I'm looking to implement:
- Full support for triangulated OBJ model loading, including matrix transforms to composite a scene.
- Refractive materials (Glass)

## Dependencies

SDL2 (Optional, but you'll have to comment a bit of code to work without it)
Standard C99/GNU99 with some standard libraries
All other libraries are included as source

## Installation

POSIX:
1. Install SDL2
2. Run 'make'
3. Suggest a fix to my makefile because it didn't link SDL2 on your platform.
4. Run binary in ./bin/
WINDOWS:
1. Open the VS project in CRayWindows
2. CTRL+F5 to run without debugging
3. VS places a binary under CRayWindows/bin/

## Tests

You really thought I wrote tests...?

## Credits

3rd party libraries included in this project

OBJ Loader library: http://www.kixor.net/dev/objloader/

lodePNG PNG compression library: http://lodev.org/lodepng/

SDL2: https://www.libsdl.org/index.php

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

