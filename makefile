BIN=./bin/
SRC=./src/
EXE_NAME = c-ray
OBJS = CRay.o vector.o color.o scene.o poly.o light.o sphere.o filehandler.o camera.o modeler.o errorhandler.o obj_parser.o string_extra.o list.o lodepng.o
CC = gcc
FLAGS = -Wall -O2
LIBS = -lm -lpthread -lSDL2

.c.o:
	$(CC) $< -c $(FLAGS) $(INC)

$(BIN)$(EXE_NAME): $(SRC)$(OBJS)
	$(CC) -o $@ $(OBJS) $(FLAGS) $(LIBS)
clean:
	rm -f $(BIN)$(OBJS) $(BIN)$(EXE_NAME)
