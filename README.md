<p align="center">
	<img src="https://i.imgur.com/fPBuCTG.png" width="256">
</p>

## Rendering with SDL installed

<p align="center">
	<img src="https://media.giphy.com/media/8cT6Dbo7kCi3rRj5Fr/giphy.gif" width="512">
</p>

## Example render (2000 samples, 1920x1080, 50 bounces, 1h 55min)

<p align="center">
	<img src="https://teensyimg.com/images/PLxxP7lRVE.png" width="768">
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
- Russian Roulette path optimization

The default recursive path tracing algorithm supports:
- metal
- glass
- lambertian diffuse
- triangles and spheres
- Depth of field
- Diffuse textures

Things I'm looking to implement:
- Built a more robust API with a scene state.
- Some procedural textures
- Expand the default path tracer to use PBR

## Usage

Please see the [Wiki](https://github.com/VKoskiv/c-ray/wiki) for details on how to use the JSON scene interface!

## Dependencies

- CMake for the build system
- SDL2 (Optional, CMake will link and enable it if it is found on your system.)
- Standard C99/GNU99 with some standard libraries

All other libraries are included as source

## Installation

macOS:
Either follow these instructions, or the instructions for Linux below. Both work fine.
1. Install SDL2 (See installing SDL below)
2. Open the .xcodeproj file in Xcode
3. Edit scheme by clicking `C-Ray` in top left, make sure 'Use custom working directory' is ticked and set it to the root directory of this project.
4. Go into the `Arguments` tab, and add by clicking `+`. Type in `./input/scene.json`, then click close
5. Uncomment the `UI_ENABLED` define at the top of `src/includes.h` if you have SDL2 installed.
6. Build&Run with `CMD+R`

Linux:
1. (Optional) Install SDL2 (See installing SDL below)
2. Run `cmake .`, or optionally `cmake . -DNO_SDL2=True` to disable SDL2.
3. Run `make` to build the project
4. Run binary: `./bin/c-ray ./input/scene.json` (Making sure the working dir is the root directory). You can also pipe files into `C-ray` and it will read from there. This is useful for scripts that invoke `C-ray`.
Example: `cat input/scene.json | ./bin/c-ray`

Windows:
1. Download SDL2 Development libaries from here and extract: https://www.libsdl.org/download-2.0.php (https://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip)
2. Open a `x64 Native Tools Command Prompt` and set path to SDL2DIR (where you extracted the files to, should have a few folders like 'include' and 'lib'): `set SDL2DIR=E:\sdl2\SDL2-devel-2.0.8-VC\SDL2-2.0.8`
3. Run cmake: `cmake -G "Visual Studio 15 2017 Win64" .`
4. (Optional) Edit `src\includes.h` to uncomment `#define UI_ENABLED` and copy your `SDL2.dll` into `bin\Release\`
5. Build the generated solution: `msbuild c-ray.sln /p:Configuration=Release`
6. Run:	`bin\Release\c-ray.exe input\scene.json` or `type input\scene.json | bin\Release\c-ray.exe`

## Installing SDL

On macOS, use `homebrew` to install SDL. `brew install sdl2`

On Windows, download from `https://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip`

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

PCG random number generator: http://www.pcg-random.org

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

You can also ping me on **Discord!**: `vkoskiv#3100`
