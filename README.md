## Synopsis

C-Ray is a simple raytracer built for studying computer graphics. The implementation is by no means the best, the most efficient nor fully standard, but it's simple, and so is the syntax (hopefully!)
I slowly began building it from bits I could find online two years ago, when I barely knew enough about the actual theory behind all this. I've learned a lot since then!

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
1. Install Cygwin
2. Run 'make'
3. Suggest a fix to my makefile because it didn't link ??? on your platform.
4. Run bunary in ./bin/

## Tests

You really thought I wrote tests...?

## Contributors

If you know more than me, please do get in touch at vkoskiv [at] gmail (dot) com!

