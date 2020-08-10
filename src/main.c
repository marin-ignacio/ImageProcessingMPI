#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <mpi.h>
#include "../include/image_processing.h"

#define DEVICES	2

const float FPS = 60;

ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;

int rank, size, name_len;
bool running = true;
bool redraw = true;
int rank_cores[DEVICES];

//------------------------------------------------------------------------------------------------------------
// Structures
//------------------------------------------------------------------------------------------------------------
struct Image
{
	char 			path[64];
	int 			height;
	int 			width;
	int 			horizontal_middle;
	int 			vertical_middle;
	ALLEGRO_BITMAP 	*data;
} 
input_image;

//------------------------------------------------------------------------------------------------------------
// Functions declaration
//------------------------------------------------------------------------------------------------------------
void load_image();
void send_pixels_to_master(const int *ptr_height, const int *ptr_width, unsigned char ***output_img);
void receive_pixels_from_client(int source, int x0, int y0, int x1, int y1, unsigned char ***output_img);

int al_initialize()
{
	// Initialize allegro
	if (!al_init()) 
	{
		fprintf(stderr, "Failed to initialize allegro.\n");
		return 1;
	}

	//Init image reader
	al_init_image_addon();
	al_init_primitives_addon();

	return 0;
}

int al_show_image(ALLEGRO_BITMAP *image)
{
	// Initialize the timer
	timer = al_create_timer(1.0 / FPS);

	if (!timer)
	 {
		fprintf(stderr, "Failed to create timer.\n");
		return 1;
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED);
	display = al_create_display(input_image.width, input_image.height);

	if (!display) 
	{
		fprintf(stderr, "Failed to create display.\n");
		return 1;
	}

	// Create the event queue
	event_queue = al_create_event_queue();

	if (!event_queue) 
	{
		fprintf(stderr, "Failed to create event queue.");
		return 1;
	}

	//configure the window
	al_set_window_title(display,"Filtered Image");

	// Register event sources
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	// Display a black screen
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// Start the timer
	al_start_timer(timer);

	while (running) 
	{ 
		ALLEGRO_EVENT 	event;
		ALLEGRO_TIMEOUT timeout;
		ALLEGRO_BITMAP 	*bitmap;

		// Initialize timeout
		al_init_timeout(&timeout, 0.06);

		// Fetch the event (if one exists)
		bool get_event = al_wait_for_event_until(event_queue, &event, &timeout);

		// Handle the event
		if (get_event)
			switch (event.type) 
			{
				case ALLEGRO_EVENT_TIMER:
					redraw = true;
					break;
				case ALLEGRO_EVENT_DISPLAY_CLOSE:
					running = false;
					break;
				default:
					fprintf(stderr, "Unsupported event received: %d\n", event.type);
					break;
			}

		// Check if we need to redraw
		if (redraw && al_is_event_queue_empty(event_queue)) 
		{
			al_clear_to_color(al_map_rgb(0, 0, 0));

			al_draw_bitmap(image, 0, 0, ALLEGRO_FLIP_VERTICAL);
			
			al_flip_display();
			redraw = false;
		}
	} 

	// Clean up
	al_destroy_display(display);
	al_destroy_event_queue(event_queue);
	al_destroy_bitmap(image);
	al_destroy_bitmap(input_image.data);

	return 0;
}

//------------------------------------------------------------------------------------------------------------
// main function
//------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    switch (rank)
    {
    	case 0:
    		for (int i = 0; i < DEVICES; i++)
			{
				printf("Enter number of cores for device %i: ", i);
				scanf("%i", &rank_cores[i]);
			}

			printf("Enter image path: ");
			scanf("%s", input_image.path);

			MPI_Send(input_image.path, 64, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
			//MPI_Send(input_image.path, 64, MPI_BYTE, 2, 0, MPI_COMM_WORLD);
			//MPI_Send(input_image.path, 64, MPI_BYTE, 3, 0, MPI_COMM_WORLD);

    		break; 

    	case 1:
    		MPI_Recv(input_image.path, 64, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    		break;

    	case 2:
    		MPI_Recv(input_image.path, 64, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    		break;

    	case 3:
    		MPI_Recv(input_image.path, 64, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    		break;

    	default:
    		break;
    }

	al_initialize();

	load_image();

	//------------------------------------------------------------------------------------------------------------
	// Splits input image in four parts
	//------------------------------------------------------------------------------------------------------------
	ALLEGRO_BITMAP *sub_img_data;
	int sub_img_width, sub_img_height;

    switch (rank)
    {
    	case 0:
    		printf("w = %i, h = %i\n", input_image.width, input_image.height);

    		//sub_img_data = al_create_sub_bitmap(input_image.data, 0, 0, input_image.horizontal_middle + 1, input_image.vertical_middle + 1);
    		sub_img_data = al_create_sub_bitmap(input_image.data, 0, 0, input_image.horizontal_middle + 1, input_image.height);
    		break; 

    	case 1:
    		//sub_img_data = al_create_sub_bitmap(input_image.data, input_image.horizontal_middle - 1, 0, input_image.width, input_image.vertical_middle + 1);
    		sub_img_data = al_create_sub_bitmap(input_image.data, input_image.horizontal_middle - 1, 0, input_image.width, input_image.height);
    		break;

    	case 2:
    		//sub_img_data = al_create_sub_bitmap(input_image.data, 0, input_image.vertical_middle - 1, input_image.horizontal_middle + 1, input_image.height);
    		break;

    	case 3:
    		//sub_img_data = al_create_sub_bitmap(input_image.data, input_image.horizontal_middle - 1, input_image.vertical_middle - 1, input_image.width, input_image.height);
    		break;

    	default:
    		break;
    }

	//Gets the dimensions of subimage
	sub_img_height = al_get_bitmap_height(sub_img_data);
    sub_img_width  = al_get_bitmap_width(sub_img_data);  

    //Creates a RGB matrix for the subimage
    unsigned char ***sub_img = createMatrix(sub_img_data);

    unsigned char ***sub_output_img = allocateMemorySpaceForImage(&sub_img_height, &sub_img_width);

    //Applies the median filter on the subimage
	medianFilter(&sub_img_height, &sub_img_width, sub_img, sub_output_img);

    printf("Rank %i filter success!\n", rank);

	//------------------------------------------------------------------------------------------------------------
	// Combines the output subimages
	//------------------------------------------------------------------------------------------------------------
	switch (rank)
    {
    	case 0:
    	{
			unsigned char ***output_img = allocateMemorySpaceForImage(&input_image.height, &input_image.width);

			printf("Rank %i (%i to %i, %i to %i)\n", rank, 0, sub_img_width, 0, sub_img_height);

			for (int i = 0; i < sub_img_height; i++)
				for (int j = 0; j < sub_img_width; j++)
					for (int c = 0; c < COLOR_CHANNELS; c++)
						output_img[i][j][c] = sub_output_img[i][j][c];

			//receive_pixels_from_client(1, input_image.horizontal_middle, 0, input_image.width, input_image.vertical_middle, output_img);
			receive_pixels_from_client(1, input_image.horizontal_middle, input_image.width, 0, input_image.height, output_img);
			//receive_pixels_from_client(2, 0, input_image.vertical_middle, input_image.horizontal_middle, input_image.height, output_img);
			//receive_pixels_from_client(3, input_image.horizontal_middle, input_image.vertical_middle, input_image.width, input_image.height, output_img);
    	
			//Creates and saves the output image file
			generateBitmapImage(input_image.height, input_image.width, output_img, "images/output_img.bmp");

			//Reads the output image image file
			ALLEGRO_BITMAP *output_img_bitmap = al_load_bitmap("images/output_img.bmp");

			al_show_image(output_img_bitmap);

			deallocateMemorySpaceForImage(input_image.height, input_image.width, output_img);
    		break; 
    	}

    	case 1:
  			send_pixels_to_master(&sub_img_height, &sub_img_width, sub_output_img);
	  		break;

    	case 2:
    		send_pixels_to_master(&sub_img_height, &sub_img_width, sub_output_img);
    		break;

    	case 3:
 			send_pixels_to_master(&sub_img_height, &sub_img_width, sub_output_img);
    		break;

    	default:
    		break;
    }

	//Deallocate reserved memory
	deallocateMemorySpaceForImage(sub_img_height, sub_img_width, sub_img);
	deallocateMemorySpaceForImage(sub_img_height, sub_img_width, sub_output_img);

	MPI_Finalize();

	return 0;
}


//------------------------------------------------------------------------------------------------------------
// Functions definition
//------------------------------------------------------------------------------------------------------------
void receive_pixels_from_client(int source, int x0, int x1, int y0, int y1, unsigned char ***output_img)
{
	int tmp_pixel;

	printf("Rank %i (%i to %i, %i to %i)\n", source, x0, x1, y0, y1);

	for (int i = y0; i < y1; i++)
		for (int j = x0; j < x1; j++)
			for (int c = 0; c < COLOR_CHANNELS; c++)
			{
				MPI_Recv(&tmp_pixel, 1, MPI_INT, source, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				output_img[i][j][c] = tmp_pixel;
			}

	printf("Rank %i received pixels from %i success!\n", rank, source);
}


void send_pixels_to_master(const int *ptr_height, const int *ptr_width, unsigned char ***sub_output_img)
{
    for (int i = 0; i < *ptr_height; i++)
		for (int j = 0; j < *ptr_width; j++)
			for (int c = 0; c < COLOR_CHANNELS; c++)
				MPI_Send(&sub_output_img[i][j][c], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
}


void load_image()
{
	//Read the bitmap from the image .png
	input_image.data = al_load_bitmap(input_image.path);

	if (!input_image.data)
	{
		printf("Error loading image path.\n");
		exit(1);
	}

    input_image.width  			  = al_get_bitmap_width(input_image.data);
	input_image.height            = al_get_bitmap_height(input_image.data);
	input_image.horizontal_middle = input_image.width  / 2;
    input_image.vertical_middle   = input_image.height / 2;
}
