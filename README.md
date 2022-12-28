<p align="center">
	<img src="https://i.imgur.com/fPBuCTG.png" width="256">
</p>

## Example renders

<p align="center">
	<img src="https://teensyimg.com/images/AJRDYdERsy.png" width="768">
	<br>(1000 samples, 1920x1080, 512 max bounces, 26min, scene by <a href="https://www.blendswap.com/blend/13953">Scott Graham</a>)
</p>
<p align="center">
	<img src="https://teensyimg.com/images/sTYqjhGMo.png" width="768">
	<br>(512 samples, 2560x1600, 30 max bounces, 8min)
</p>

## Status

[![Build Status](https://github.com/VKoskiv/c-ray/workflows/C-ray%20CI/badge.svg)](https://github.com/VKoskiv/c-ray/actions?query=workflow%3A%22C-ray+CI%22)
[![justforfunnoreally.dev badge](https://img.shields.io/badge/justforfunnoreally-dev-9ff)](https://justforfunnoreally.dev)

## Synopsis

C-ray is a research oriented, hackable, offline CPU rendering engine built for learning. The source code is intended to be readable wherever possible, so feel free to explore and perhaps even expand upon the current functionality. See the [contributing section](https://github.com/vkoskiv/c-ray/wiki/Contributing) in the wiki for more details.

C-ray currently has:
- [Cluster rendering support](https://github.com/vkoskiv/c-ray/wiki/Using-cluster-rendering) (on \*nix systems)
- A node-based material system
- A simple unidirectional Monte Carlo integrator
- Real-time render preview using SDL2
- Easy scene compositing using a JSON interface
- Multithreading (using pthreads or WIN32 threads)
- Wavefront OBJ support
- Object instancing
- PNG and BMP file output
- Two levels of BVH acceleration structures (by @madmann92)
- Antialiasing (MSAA)
- HDR environment maps for realistic lighting
- Gouraud interpolated smooth shading
- Benchmarking metrics

The default integrator supports:
- Metal
- Glass
- Lambertian diffuse
- Plastic
- Triangles and spheres
- Depth of field
- Russian Roulette path optimization
- Diffuse textures
- Normal maps

Things I'm looking to implement:
- Proper physically based materials in place of the current ad-hoc implementations
- Built a more robust API with an interactive scene state.
- Some procedural textures
- Python API wrapper
- Volumetric rendering
- Subsurface scattering
- Spectral rendering

## Compatibility

C-ray has been verified to work on the following architectures
- x86 & x86\_64 (Primarily developed on x86\_64)
- ARMv6, ARMv7 and ARMv8 (Various Raspberry Pi systems)
- PowerPC 7xx and 74xx (PowerPC G3 and G4 systems)
- MIPS R5000 ([1996 SGI O2](https://twitter.com/vkoskiv/status/1236419126555488257?s=20))
- SuperSPARC II ([1992 SUN SparcStation 10](https://twitter.com/vkoskiv/status/1234515380200235008?s=20))

## Usage

Please see the [Wiki](https://github.com/VKoskiv/c-ray/wiki) for details on how to use the JSON scene interface.

## Dependencies

- CMake for the build system (Optional, a basic makefile is provided for *nix systems)
- SDL2 (Optional, CMake will link and enable it if it is found on your system.)
- Python3 (Optional, it's used for some utility scripts)
- Standard C99/GNU99 with some standard libraries

All other dependencies are included as source

## Installation

macOS:
Either follow these instructions, or the instructions for Linux below. Both work fine.
1. Install SDL2 (See installing SDL below)
2. Open the .xcodeproj file in Xcode
3. Edit scheme by clicking `C-Ray` in top left, make sure 'Use custom working directory' is ticked and set it to the root directory of this project.
4. Go into the `Arguments` tab, and add by clicking `+`. Type in `./input/scene.json`, then click close
5. Build&Run with `CMD+R`

Linux:
1. (Optional) Install SDL2 (See installing SDL below)
2. (Optional, this tends to work nicer) Run `cmake .`, or optionally `cmake . -DNO_SDL2=True` to disable SDL2.
3. Run `make` to build the project
4. Run binary: `bin/c-ray input/scene.json` (Making sure the working dir is the root directory). You can also pipe files into `C-ray` and it will read from there. This is useful for scripts that invoke `C-ray`.
Example: `cat input/scene.json | bin/c-ray`
*Note: Reading the json from `stdin` assumes that the asset path is `./`, can be specified with `--asset-path`*

Windows:
1. Install [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019)
2. Optional: Download SDL2 Development libaries from here and extract: https://www.libsdl.org/download-2.0.php (https://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip)
3. Open a `Developer Command Prompt for VS 2019`, navigate to where you cloned c-ray and set path to SDL2DIR (where you extracted the files to, should have a few folders like 'include' and 'lib'): `set SDL2DIR=E:\sdl2\SDL2-devel-2.0.8-VC\SDL2-2.0.8`
4. Run cmake: `cmake -G "Visual Studio 16 2019" .`
5. (Optional) Copy your `SDL2.dll` into `bin\Release\` and `bin\Debug\`
6. Build the generated solution: `msbuild c-ray.sln /p:Configuration=Release`
7. Run:	`bin\Release\c-ray.exe input\scene.json` or `type input\scene.json | bin\Release\c-ray.exe`

## Usage

All the .json files in `input/` are test scenes provided with c-ray, assets for those scenes are (mostly) bundled with the repository as well.

If you make a cool scene and have Python3 installed, you can bundle up the scene into a portable .zip file using the `scripts/bundle.py` script.

## Installing SDL

On macOS, use `homebrew` to install SDL. `brew install sdl2`

On Windows, download from `https://www.libsdl.org/release/SDL2-devel-2.0.8-VC.zip`

On Linux using APT, run `sudo apt install libsdl2-dev`

## Tests

You can run the integrated test suite by invoking the test script like this:
`./run-tests.sh`
This will compile C-ray with the correct flags, and then run each test individually in separate processes. If you want to run them in a shared process, do
`./bin/c-ray --test`

In either case, you will get a log of all the tests, and their status.

## Credits

3rd party libraries included in this project

lodePNG PNG compression library: http://lodev.org/lodepng/

stb\_image.h by Sean Barrett: https://github.com/nothings/stb/blob/master/stb\_image.h

SDL2: https://www.libsdl.org/index.php (Optional)

JSON parsing library: https://github.com/DaveGamble/cJSON

PCG random number generator: http://www.pcg-random.org

## Contributors

- Huge thanks to [@madmann91](https://github.com/madmann91) for the new BVH accelerator and overall ~60% performance improvement!

Please file an issue detailing any improvements you're planning on making. I openly welcome contributions!

You can also ping me on **Discord**: `vkoskiv#3100`
