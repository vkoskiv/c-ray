## Status

[![Build Status](https://semaphoreci.com/api/v1/vkoskiv/c-ray/branches/master/badge.svg)](https://semaphoreci.com/vkoskiv/c-ray)

## Synopsis

C-Ray is a simple raytracer built for studying computer graphics. It's also a great platform for developing your own raytracing algorithms. Just write your own rayTrace() function! Multithreading, 3D model loading and render previews are handled by c-ray, so you can concentrate on the core principles.

C-Ray currently supports:
- Real-time render preview using SDL
- Multithreading
- OBJ loading with matrix transforms for compositing a scene
- PNG and BMP file output
- k-d tree acceleration structure for fast intersection checks even with millions of primitives
- Antialiasing
- Multi-sampling

The default recursive raytracing algorithm supports:
- reflections
- lights with radius (soft shadows)
- triangles and spheres
- blinn-phong lighting model
- Depth of field
- Refraction

Things I'm looking to implement:
- Implement more robust material handling to support textures, and multiple materials for a single OBJ
- Some procedural textures
- Expand the default raytracer into a full path tracer
- Expand the default raytracer to use PBR

## Dependencies

- SDL2 (Disabled by default. uncomment #define UI_ENABLED in includes.h:12 to enable it)
- Standard C99/GNU99 with some standard libraries

All other libraries are included as source

## Installation

macOS:
1. Install SDL2 (See installing SDL below)
2. Open the .xcodeproj file in Xcode
3. Edit scheme by clicking `C-Ray` in top left, make sure 'Use custom working directory' is ticked and set it to the root directory of this project.
4. Build&Run with `CMD+R`

Linux:
1. Install SDL2 (See installing SDL below)
2. Run `make`
3. Suggest a fix to my makefile because it didn't link SDL2 on your platform.
4. Run binary: `./bin/c-ray` (Making sure the working dir is the root directory)

Windows:
1. Open the VS project in CRayWindows
2. `CTRL+F5` to run without debugging
3. VS places a binary under `CRayWindows\bin\`
i
## Installing SDL

On macOS, download the SDL2 runtime framework from https://www.libsdl.org/download-2.0.php and place in `/Library/Frameworks/`

If you don't have root access, place under `~/Library/Frameworks`

On Windows, Visual Studio should include SDL automatically

On Linux using APT, run `sudo apt install libsdl2-dev`

## Writing your own raytracing algorithm

C-Ray is built such that a new raytracing algorithm can be a drop-in replacement. Here's some pointers on getting started.

Your custom raytracing function takes two parameters. A light ray, and a scene:
`struct color rayTrace(struct lightRay *incidentRay, struct world *scene)`
It then returns a color corresponding to that pixel.

The `renderThread` in `renderer.c` runs on multiple threads, and sets up the light ray, and passes it to the function you write. This call happens at `renderer.c:386`

Note: Currently there are *two* default renderers. `rayTrace()` is the original algorithm I wrote starting in 2015, and `newTrace()` is a new recursive, more modular algorithm that I hope to expand into a full PBR path tracer eventually.

When writing your own algorithm, either replace one of the existing ones, or write your own into raytrace.c and edit the function call on `renderer.c:386`

Use the built-in intersection checks in your algorithm:

`rayIntersectsWithNode()` takes the kd-tree root of an OBJ, a `lightRay`, and an `intersection` struct. It returns true if intersected, and sets useful shading information into the given `intersection` struct. This function uses the kd-tree acceleration structure to minimize the amount of intersection checks.

If you do want to check for an intersection on a primitive (Without acceleration structure), use
`rayIntersectsWithPolygon()`. It takes a `lightRay`, a `poly`, current `t` value, a surface normal and a UV coordinate. Returns true if intersected, and sets the surface normal and UV to the given pointers.

For spheres, we have similar functions:
`rayIntersectsWithSphere()` takes a `lightRay`, a `sphere` and the current `t` value, and returns true if intersected.

`rayIntersectsWithSphereTemp()` is a newer function that sets shading information into a given `intersection` struct, just like `intersectsWithNode`. It uses `rayIntersectsWithSphere` internally.

Currently, the scene is defined in a central place in the code: The `testBuild()` function in `scene.c:201`
I want to implement a config parser, or a launcher to set these in the future.

## Tests

You really thought I wrote tests...?

## Credits

3rd party libraries included in this project

OBJ Loader library: http://www.kixor.net/dev/objloader/

lodePNG PNG compression library: http://lodev.org/lodepng/

SDL2: https://www.libsdl.org/index.php

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

