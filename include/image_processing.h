#ifndef WINDOWLOGIC_H
#define WINDOWLOGIC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#define COLOR_CHANNELS 3 //RED,GREEN,BLUE
#define R_CHANNEL 2
#define G_CHANNEL 1
#define B_CHANNEL 0
#define WINDOW_FILTER_SIZE 3
#define fileHeaderSize 14
#define infoHeaderSize 40


unsigned char *** createMatrix(ALLEGRO_BITMAP *image);

unsigned char *** allocateMemorySpaceForImage(const int *ptr_height, const  int *ptr_width);

void medianFilter(const int *ptr_height, const int *ptr_width, unsigned char ***input_img, unsigned char ***output_img);

int calculateMedian(unsigned char * arr, int length);

void generateBitmapImage(int height, int width, unsigned char *** image, char * imageFileName);

unsigned char * createBitmapFileHeader(int height, int width, int paddingSize);

unsigned char * createBitmapInfoHeader(int height, int width);

void deallocateMemorySpaceForImage(int height, int width, unsigned char *** image_matrix);

#endif