CC=gcc
CFLAGS=-I.
DEPS = CRay.h includes.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cray: Cray.c camera.c color.c errorhandler.c filehandler.c modeler.c poly.c scene.c sphere.c vector.c 
	$(CC) -o cray CRay.c camera.c color.c errorhandler.c filehandler.c modeler.c poly.c scene.c sphere.c vector.c -I.
