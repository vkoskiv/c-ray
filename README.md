<p align="center">
	<img src="https://i.imgur.com/fPBuCTG.png" width="256">
</p>

# c-ray

A portable, hackable, embeddable software path tracer.

## Status

[![Build Status](https://github.com/VKoskiv/c-ray/workflows/C-ray%20CI/badge.svg)](https://github.com/VKoskiv/c-ray/actions?query=workflow%3A%22C-ray+CI%22)
[![justforfunnoreally.dev badge](https://img.shields.io/badge/justforfunnoreally-dev-9ff)](https://justforfunnoreally.dev)

## Example renders

<p align="center">
	<img src="https://teensyimg.com/images/AJRDYdERsy.png" width="768">
	<br>(1000 samples, 1920x1080, 512 max bounces, 26min, scene by <a href="https://www.blendswap.com/blend/13953">Scott Graham</a>)
</p>
<p align="center">
	<img src="https://teensyimg.com/images/fuexHP7SiJ.png" width="768">
	<br>(256 samples, 2560x1440, 12 max bounces, 9min 37s, scene by <a href="https://www.blendswap.com/blend/18762">MaTTeSr</a>)
</p>
<p align="center">
	<img src="https://teensyimg.com/images/sTYqjhGMo.png" width="768">
	<br>(512 samples, 2560x1600, 30 max bounces, 8min)
</p>

## About

c-ray is a portable, hackable, offline CPU rendering engine built for learning. The core is in plain C99, with an emphasis on clarity and avoiding superfluous abstraction. Contributions are welcome. See the [contributing section](https://github.com/vkoskiv/c-ray/wiki/Contributing) in the wiki for more details.

An incomplete list of features:
- [Cluster rendering support over TCP/IP](https://github.com/vkoskiv/c-ray/wiki/Using-cluster-rendering)
- [A C API](https://github.com/vkoskiv/c-ray/blob/master/include/c-ray/c-ray.h)
- [Python bindings](https://github.com/vkoskiv/c-ray/blob/master/bindings/c_ray.py)
- [A Blender add-on](https://github.com/vkoskiv/c-ray/blob/master/bindings/blender_init.py)
- A node-graph material/shader system with 32+ node types, including a Principled BSDF approximation.
- A performant BVH accelerator (by @madmann92)
- A simple unidirectional Monte Carlo integrator with global illumination
- Real-time render preview and state reporting using a callback mechanism
- Simple thin-lens camera approximation with depth of field
- Multithreading
- Object instancing
- HDR environment maps for realistic lighting
- Triangles and spheres
- Russian Roulette path optimization

Things I'm looking to implement:
- More advanced light sampling
- Better performance & lower memory consumption.
- Proper physically based materials in place of the current ad-hoc implementations
- More cool advanced techniques from research literature

## Portability

c-ray has been verified to work on the following architectures
- x86 & x86\_64 (Primarily developed on x86\_64)
- ARMv6, ARMv7 and ARMv8, AARCH64 (Various Raspberry Pi systems)
- PowerPC 7xx and 74xx (PowerPC G3 and G4 systems)
- MIPS R5000 ([1996 SGI O2](https://twitter.com/vkoskiv/status/1236419126555488257?s=20))
- SuperSPARC II ([1992 SUN SparcStation 10](https://twitter.com/vkoskiv/status/1234515380200235008?s=20))
- [WebAssembly with emscripten](https://github.com/ani003/c-ray/tree/wasm)

## Usage

A basic driver program can be used to run c-ray standalone (see 'Stand-alone usage' below), but the easiest way to try out c-ray is to build and install the Blender add-on, and trying it out there:

1. Check `BLENDER_ROOT` in `lib.mk`, make sure it points to the version of Blender you have installed
2. `make fullblsync` will then compile the python bindings (`cray_wrap.so`), and install under `BLENDER_ROOT` as an add-on.
3. Enable the c-ray add-on in Blender Preferences, and choose `c-ray for Blender` as your render engine.
4. Report bugs or missing features (there are a lot!) by filing an issue here on GitHub

## Dependencies

### Compile time:

- Standard C99 compiler with some fairly common libraries (libc, libm, pthreads)

### Runtime:
- CMake for the build system (Optional, a basic makefile is provided for *nix systems)
- SDL2 (Optional, enabled if SDL2 was found at runtime)
- Python3 (Optional, it's used for some utility scripts)

## Tests

You can run the integrated test suite by invoking the test script like this:
`./run-tests.sh`
This will compile c-ray with the correct flags, and then run each test individually in separate processes. If you want to run them in a shared process, do
`bin/c-ray --test`
You can also run a single suite
`./run-tests.sh mathnode`

## Stand-alone usage

You can mostly ignore these instructions below if you're only interested in running c-ray as a Blender add-on.

Linux:
1. (Optional) Install SDL2 (See installing SDL below)
2. Run `make` to build the project
3. If the plain Makefile doesn't work on your system, run `cmake .` and then try `make` again.
4. Run binary. For example: `bin/c-ray input/hdr.json`. You can also pipe files into `c-ray` and it will read from there. This is useful for scripts that invoke `c-ray`.
Example: `cat input/scene.json | bin/c-ray`
*Note: When reading the json from `stdin`, c-ray assumes that the asset path is `./`. This can be specified with `--asset-path`*

macOS:
1. Follow Linux instructions, or you can try `mkdir build && cd build && cmake -G Xcode ..`

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

## Credits

3rd party libraries included in this project include:

- lodePNG PNG compression library: http://lodev.org/lodepng/
- stb\_image.h by Sean Barrett: https://github.com/nothings/stb/blob/master/stb\_image.h
- SDL2: https://www.libsdl.org/index.php (Optional)
- JSON parsing library: https://github.com/DaveGamble/cJSON
- PCG random number generator: http://www.pcg-random.org

## Contributors

- Huge thanks to [@madmann91](https://github.com/madmann91) for the BVH accelerator and overall ~60% performance improvement

Please file an issue detailing any improvements you're planning on making. I openly welcome contributions!

You can also ping me on **Discord**: `vkoskiv#3100`
