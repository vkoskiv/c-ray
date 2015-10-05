CC=gcc
CFLAGS=-I.
DEPS = CRay.h includes.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cray: CRay.o
	$(CC) -o cray CRay.o -I.
