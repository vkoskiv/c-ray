<p align="center">
	<img src="https://i.imgur.com/fPBuCTG.png" width="256">
</p>

## Example renders

<p align="center">
	<img src="https://teensyimg.com/images/qv0JtPxJ7c.png" width="768">
	<br>(2500 samples, 2560x1600, 512 max bounces, 1h 22min)
</p>
<p align="center">
	<img src="https://teensyimg.com/images/AJRDYdERsy.png" width="768">
	<br>(1000 samples, 1920x1080, 512 max bounces, 1h 23min, scene by <a href="https://www.blendswap.com/blend/13953">Scott Graham</a>)
</p>

## Status

[![Build Status](https://github.com/VKoskiv/c-ray/workflows/C-ray%20CI/badge.svg)](https://github.com/VKoskiv/c-ray/actions?query=workflow%3A%22C-ray+CI%22)

## Synopsis

C-ray is a research oriented, physically accurate rendering engine built for learning. The source code is intended to be readable wherever possible, so feel free to explore and perhaps even expand upon the current functionality. See the contributing section in the wiki for more details.

C-ray currently has:
- A simple unidirectional unbiased integrator
- Real-time render preview using SDL
- Easy scene compositing using JSON
- Multithreading (pthreads and WIN32 threads)
- OBJ loading with transforms for compositing a scene
- PNG and BMP file output
- BVH acceleration structures by @madmann91
- Antialiasing
- Multi-sampling
- HDR environment maps for realistic lighting
- Gouraud interpolated smooth shading
- Benchmarking metrics

The default integrator supports:
- metal
- glass
- lambertian diffuse
- plastic
- triangles and spheres
- Depth of field
- Russian Roulette path optimization
- Diffuse textures

Things I'm looking to implement:
- Built a more robust API with a scene state.
- Some procedural textures
- Python API wrapper
- Better materials
- Networking and clustered rendering
- Volumetric rendering
- Subsurface scattering
- Spectral rendering

## Compatibility

C-ray has been verified to work on the following architectures
- x86 & x86_64 (Primarily developed on x86_64)
- ARMv6, ARMv7 and ARMv8 (Various Raspberry Pi systems)
- PowerPC 7xx and 74xx (PowerPC G3 and G4 systems)
- MIPS R5000 (1996 SGI O2)
- SuperSPARC II (1992 SUN SparcStation 10)

## Usage

Please see the [Wiki](https://github.com/VKoskiv/c-ray/wiki) for details on how to use the JSON scene interface!

## Dependencies

- CMake for the build system
- SDL2 (Optional, CMake will link and enable it if it is found on your system.)
- Python3 (Optional, it's used for some utility scripts)
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

## Tests

No tests yet! Maybe soon!

## Credits

3rd party libraries included in this project

OBJ Loader library: http://www.kixor.net/dev/objloader/

lodePNG PNG compression library: http://lodev.org/lodepng/

stb\_image.h by Sean Barrett: https://github.com/nothings/stb/blob/master/stb_image.h

SDL2: https://www.libsdl.org/index.php (Optional)

JSON parsing library: https://github.com/DaveGamble/cJSON

PCG random number generator: http://www.pcg-random.org

## Contributors

- @madmann91 - BVH accelerator and overall ~60% performance improvement. Thanks!

Please file an issue detailing any improvements you're planning on making. I openly welcome contributions!

You can also ping me on **Discord**: `vkoskiv#3100`
