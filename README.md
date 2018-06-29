<p align="center">
	<img src="https://i.imgur.com/fPBuCTG.png" width="256">
</p>

## Rendering with SDL installed

<p align="center">
	<img src="https://media.giphy.com/media/8cT6Dbo7kCi3rRj5Fr/giphy.gif" width="512">
</p>

## Example render (2000 samples, 11 hours)

<p align="center">
	<img src="https://teensyimg.com/images/urzClV90yC.png" width="768">
</p>

## Status

[![Build Status](https://semaphoreci.com/api/v1/vkoskiv/c-ray/branches/master/badge.svg)](https://semaphoreci.com/vkoskiv/c-ray)

## Synopsis

C-ray is a simple path tracer built for studying computer graphics. It's also a great platform for developing your own raytracing algorithms. Just write your own rayTrace() function! Multithreading, 3D model loading and render previews are handled by C-ray, so you can concentrate on the core principles.

C-ray currently supports:
- Real-time render preview using SDL
- Easy scene compositing using JSON
- Multithreading
- OBJ loading with matrix transforms for compositing a scene
- PNG and BMP file output
- k-d tree acceleration structure for fast intersection checks even with millions of primitives
- Antialiasing
- Multi-sampling

The default recursive path tracing algorithm supports:
- metal
- lambertian diffuse
- triangles and spheres
- Depth of field

Things I'm looking to implement:
- Implement more robust material handling to support textures, and multiple materials for a single OBJ
- Some procedural textures
- Expand the default path tracer to use PBR

## Usage

Please see the [Wiki](https://github.com/VKoskiv/c-ray/wiki) for details on how to use the JSON scene interface!

## Dependencies

- SDL2 (Disabled by default. uncomment #define UI_ENABLED in includes.h:12 to enable it)
- Standard C99/GNU99 with some standard libraries

All other libraries are included as source

## Installation

macOS:
1. Install SDL2 (See installing SDL below)
2. Open the .xcodeproj file in Xcode
3. Edit scheme by clicking `C-Ray` in top left, make sure 'Use custom working directory' is ticked and set it to the root directory of this project.
4. Go into the `Arguments` tab, and add by clicking `+`. Type in `./input/scene.json`, then click close
5. Build&Run with `CMD+R`

Linux:
1. (Optional) Install SDL2 (See installing SDL below)
2. (Optional) Edit `src/includes.h` to uncomment `#define UI_ENABLED`
2. Run `make`
3. Suggest a fix to my makefile because it didn't link SDL2 on your platform.
4. Run binary: `./bin/c-ray ./input/scene.json` (Making sure the working dir is the root directory)

Windows:
1. Open the VS project in CRayWindows
2. (Optional) Edit `src\includes.h` to uncomment `#define UI_ENABLED`
3. Build the project (You only need to do this once)
4. VS places a binary under `CRayWindows\bin\`
5. Open `cmd` and navigate to `C:\path\to\c-ray`
6. Run `>CRayWindows\x64\Release\CRayWindows.exe input\scene.json`

## Installing SDL

On macOS, download the SDL2 runtime framework from https://www.libsdl.org/download-2.0.php and place in `/Library/Frameworks/`

If you don't have root access, place under `~/Library/Frameworks`

On Windows, Visual Studio should include SDL automatically

On Linux using APT, run `sudo apt install libsdl2-dev`

## Writing your own raytracing algorithm

See [writing your own rayTrace()](https://github.com/VKoskiv/c-ray/wiki/Writing-your-own-rayTrace()-function)

## Tests

No tests yet! Maybe soon!

## Credits

3rd party libraries included in this project

OBJ Loader library: http://www.kixor.net/dev/objloader/

lodePNG PNG compression library: http://lodev.org/lodepng/

SDL2: https://www.libsdl.org/index.php

JSON parsing library: https://github.com/DaveGamble/cJSON

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

You can also ping me on **Discord!**: `vkoskiv#3100`
