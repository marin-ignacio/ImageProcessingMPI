CC = mpicc
RUN = mpirun
MAKE_STATIC_LIB = ar rv
ALLEGRO_FLAGS = -lallegro -lallegro_image -lallegro_primitives
LIB = cd ./lib &&
RM_O = cd ./lib && rm *.o

.PHONY: all

all: main
#	$(RM_O)

main:
	$(CC) -o ./bin/main ./src/main.c ./src/image_processing.c $(ALLEGRO_FLAGS) -lm
	$(RUN) --hostfile hosts.config ./bin/main
