CC = mpicc
RUN = mpirun
MAKE_STATIC_LIB = ar rv
ALLEGRO_FLAGS = -lallegro -lallegro_image -lallegro_primitives
LIB = cd ./lib &&
RM_O = cd ./lib && rm *.o

.PHONY: all

all: main

main:
	$(CC) ./src/main.c ./src/image_processing.c $(ALLEGRO_FLAGS) -lm -o ./bin/main
	$(RUN) -np 2 ./bin/main
