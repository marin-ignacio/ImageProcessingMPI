#include <stdio.h>
#include <unistd.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <stdbool.h>
#include <allegro5/allegro_primitives.h>
#include <mpi.h>
#include "../include/image_processing.h"

const float FPS = 60;

//define bitmapss
ALLEGRO_BITMAP *image;
ALLEGRO_DISPLAY *display = NULL;
ALLEGRO_EVENT_QUEUE *event_queue = NULL;
ALLEGRO_TIMER *timer = NULL;

int rank, size, name_len, image_width, image_height;
bool running = true;
bool redraw = true;
char image_path[256];

void al_initialize()
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

	if (rank == 0)
	{
		printf("Write image path: ");
		scanf("%s", image_path);
		MPI_Send(image_path, 256, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
	} 
	else 
	{
		MPI_Recv(image_path, 256, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}
	
	//Read the bitmap from the image .png
	image = al_load_bitmap(image_path);

	if (!image)
	{
		printf("Error loading image path.\n");
		exit(1);
	}

	image_height = al_get_bitmap_height(image);
    image_width  = al_get_bitmap_width(image);

	// Initialize the timer
	timer = al_create_timer(1.0 / FPS);

	if (!timer)
	 {
		fprintf(stderr, "Failed to create timer.\n");
		return 1;
	}

	al_set_new_display_flags(ALLEGRO_WINDOWED);
	display = al_create_display(image_width, image_height);

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
	al_set_window_title(display,"Alien Community");

	// Register event sources
	al_register_event_source(event_queue, al_get_display_event_source(display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));

	// Display a black screen
	al_clear_to_color(al_map_rgb(0, 0, 0));
	al_flip_display();

	// Start the timer
	al_start_timer(timer);
}

void al_show_image()
{
	while (running) 
	{ 
		ALLEGRO_EVENT event;
		ALLEGRO_TIMEOUT timeout;
		ALLEGRO_BITMAP *bitmap;

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
}



int main(int argc, char *argv[])
{
	int sub_bitmap_width, sub_bitmap_height;

	char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name, &name_len);

	al_initialize();

	ALLEGRO_BITMAP *sub_bitmap;

	unsigned char ***sub_bitmap_matrix, ***filtered_matrix;

	if(rank == 0)
		sub_bitmap = al_create_sub_bitmap(image, 0, 0, image_width, image_height/2 + 1);

	else 
		sub_bitmap = al_create_sub_bitmap(image, 0, (image_height/2) - 1, image_width, image_height);

	sub_bitmap_height = al_get_bitmap_height(sub_bitmap);
    sub_bitmap_width  = al_get_bitmap_width(sub_bitmap);  

	sub_bitmap_matrix = createMatriz(sub_bitmap);
	filtered_matrix = allocateMemorySpaceForImage(&sub_bitmap_height, &sub_bitmap_width);
	medianFilter(&sub_bitmap_height, &sub_bitmap_width, sub_bitmap_matrix, filtered_matrix);

	if(rank == 0)
	{
		unsigned char *** complete_matrix = allocateMemorySpaceForImage(&image_height, &image_width);

		for (int i = 0; i < (sub_bitmap_height - 1); ++i)
		{
			for (int j = 0; j < sub_bitmap_width; ++j)
			{
				for (int color = 0; color < 3; ++color)
				{
					complete_matrix[i][j][color] = filtered_matrix[i][j][color];
				}
			}
		}

		int temp_pixel;
		for (int i = (sub_bitmap_height - 1); i < image_height; ++i)
			for (int j = 0; j < sub_bitmap_width; ++j)
				for (int color = 0; color < 3; ++color)
				{
					MPI_Recv(&temp_pixel, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					complete_matrix[i][j][color] = temp_pixel;
				}
			
		
		generateBitmapImage(image_height, image_width, complete_matrix, "images/img.bmp");
		//Read the bitmap from the original image
		image = al_load_bitmap("images/img.bmp");
		al_show_image();
		deallocateMemorySpaceForImage(image_height, image_height, complete_matrix);
	} 

	else 
	{
		for (int i = 1; i < sub_bitmap_height; ++i)
			for (int j = 0; j < sub_bitmap_width; ++j)
				for (int color = 0; color < 3; ++color)
					MPI_Send(&filtered_matrix[i][j][color], 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	}

	deallocateMemorySpaceForImage(sub_bitmap_height, sub_bitmap_width, sub_bitmap_matrix);
	deallocateMemorySpaceForImage(sub_bitmap_height, sub_bitmap_width, filtered_matrix);

	MPI_Finalize();

	return 0;
}
