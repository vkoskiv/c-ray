EXE_NAME = c-ray
OBJS = CRay.o vector.o color.o scene.o poly.o light.o sphere.o filehandler.o camera.o modeler.o errorhandler.o obj_parser.o string_extra.o list.o lodepng.o
CC = gcc
FLAGS = -Wall -O2
LIBS = -lm -lpthread

.c.o:
	$(CC) $< -c $(FLAGS) $(INC)

$(EXE_NAME): $(OBJS)
	$(CC) -o $@ $(OBJS) $(FLAGS) $(LIBS)
clean:
	rm -f $(OBJS) $(EXE_NAME)