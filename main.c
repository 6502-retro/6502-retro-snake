#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "6502retro_lib.h"
#include "vdp.h"
#include "bios.h"

/*
* A 1536 byte buffer to hold the head and tail positions of the snake.
* As the snake grows, the distance between the head and tail will grow too.
* The snake grows by 5 pixels every apple eaten.  This means that the buffer must support
* at least 1536 / (size of Vec2) (768) apples before the head wraps and overtakes the tail.
* Maximum length of snake is 3840.  Max pixels at 64x48 is 3072, so this buffer size is fine.
*/
#define BUFFERLEN 0x600
#define GROWLEN 5	 // How many segments should the snake grow when it eats an apple
#define GAMESPEED 4	// game runs at ~15 frames per second

enum e_dir {
	NORTH,		// 0
	EAST,
	SOUTH,
	WEST
};

// Vector2 type
typedef struct {
	int8_t x;
	int8_t y;
} Vec2;

// Snake structure.  As the state of the snake is held in the framebuffer,
// We only need to keep the head and tail and it's current grow value.
// The head and tail are pointers into a circular buffer that contain the Vec2 points
// describing their locations.
typedef struct {
	Vec2 *head;	// pointer to the head of the snake
	Vec2 *tail;	// pointer to the tail of the snake
	uint8_t grow;	// gamespeed value for growing the snake.  Non zero means snake is growing

} Snake;

Vec2 buffer[BUFFERLEN] = {0};	// circular buffer of vector 2 locations.

// Use globals for CA65 performance
size_t seed = 0;
Vec2 apple;
Snake *snake;
uint8_t dir, k;
uint8_t gamespeed = 4;
uint8_t running = false;
uint16_t score = 0;

// Emit a new line
void crlf()
{
	bios_puts("\r\n");
}

// Return to OS
void quit()
{
	reset_interrupt();
	bios_wboot();
}

// Emit a message and return to OS
// Also print the score
void fatal(char *msg)
{
	bios_puts(msg);
	crlf();
	printf("SCORE: %d", score);
	crlf();
	quit();
}

// flush the screen if required by drawflag == true when the VDP interrupt fires
void interrupt()
{
	if (drawflag)
	{
		vdp_flush(&FRAMEBUF);
		drawflag = false;	// reset drawflag after completion
	}
}

// Wait for t/60 seconds.
void delay(uint8_t t)
{
	do {
		vdp_wait();
	} while (--t > 0);
}

// Draw a new apple on the screen.  Return the apple as a Vector 2.
Vec2 new_apple()
{
	bool taken = true;	// flag used to break out of the loop
	Vec2 a;
	// Keep looping until the attempt to draw the apple did not collide
	// with anything else in the framebuffer.
	while (taken)
	{
		a.x = (rand() & 0xFF) % 64;	// Get random X
		a.y = (rand() & 0xFF) % 48;	// Get Random Y
		if (!vdp_plot_xy(a.x,a.y,VDP_LIGHT_GREEN)) // Try to draw the apple at
			taken = false;			   // XY and if no collide, break
	}
	return a;	// Return the apple Vec2
}

// Initializes the snake to it's starting position, length and direction.
Snake *new_snake()
{
	Snake *s;
	Vec2 v;
	uint8_t i;
	memset(&buffer, 0, BUFFERLEN);

	for (i=0; i<5; ++i) 	// add 5 segments to the circular buffer
	{
		v.x = 27+i;
		v.y = 24;
		buffer[i] = v;
	}
	s->head= &buffer[i-1];
	s->tail= &buffer[0];
	s->grow = 0;		// by default the snake is not growing.
	return s;
}

// Move the snake in the direction of travel by adding a new segment into the
// circular buffer
// Check for wall collisions
// Advance the head pointer
// Advance the tail pointer if snake is not growing else decrement snake grow value.
void move_snake(Snake *s, uint8_t dir)
{
	Vec2 newseg;
	newseg = *s->head;
	switch (dir) {
		case NORTH:
			newseg.y--;
			break;
		case EAST:
			newseg.x++;
			break;
		case SOUTH:
			newseg.y++;
			break;
		case WEST:
			newseg.x--;
			break;
		default:
			break;
	}

	if (	   (newseg.x > 63)
		|| (newseg.x < 0)
		|| (newseg.y > 47)
		|| (newseg.y < 0)
	)
	{
		fatal("CRASHED INTO WALL");
	}

	if (s->head < &buffer[BUFFERLEN]-sizeof(Vec2))
		s->head++;
	else
		s->head = &buffer[0];

	if (s->grow == 0)
	{
		if (s->tail < &buffer[BUFFERLEN]-sizeof(Vec2))
			s->tail++;
		else
			s->tail = &buffer[0];
	}
	else
	{
		s->grow--;
	}

	*s->head = newseg;
}

// Draw the tail in BLACK (Deletes the last segment from the screen)
// Draw the head in snake colour (dark red here) and return collision status.
// Note: We do not need to draw the whole snake.  Just the new head position and
// remove the old tail position.
bool draw_snake(Snake *s)
{
	vdp_plot_xy(s->tail->x, s->tail->y, VDP_BLACK);
	return vdp_plot_xy(s->head->x, s->head->y, VDP_DARK_RED);
}

// Main entry point of the snake game.
void main()
{
	vdp_reset();
	vdp_colorize(VDP_BLACK);
	set_interrupt(&interrupt);	// Our user defined interrupt routine.
	crlf();

	// Increment the seed seed until the user presses SPACE.
	// This provides some semblance of randomness between plays
	bios_puts("Press SPACE to play...\r\n");
	while (bios_const() != ' ') {
		++seed;
	}

	running = true; 		// Game play continues until `running` is false
	srand(seed);			// Init random number generator with collected seed
	snake = new_snake();		// Init snake
	dir = EAST;			// Initial direction is always EAST
	draw_snake(snake);		// Draw snake once
	apple = new_apple();		// Now find an apple location - in this order so that the snake does
	                                // not overwrite the apple

	while (running)
	{
		delay(1);		// game speed.  We pause 1/60th of a second and decrement the gamespeed
		--gamespeed;              // We then collect user input and set direction accordingly.
		k = bios_const();
		switch (k) {
			case 'a':
				if (dir != EAST) dir = WEST; break;
			case 's': 
				if (dir != NORTH) dir = SOUTH; break;
			case 'd':
				if (dir != WEST) dir = EAST; break;
			case 'w':
				if (dir != SOUTH) dir = NORTH; break;
			case 'q': running = false;
			default: break;
		}
		if (gamespeed == 0)	 // When gamespeed is zero, we move the snake and draw the snake.
		{	                // Drawing the head of the snake returns true if it collided 
			                // with anything on the screen.
			move_snake(snake, dir);
			if (draw_snake(snake))
			{
				if (	// check if the collision was with an apple?
					(snake->head->x == apple.x) &&
					(snake->head->y == apple.y)
				)
				{	// Draw new apple and start growing the snake.  The tail is not
					apple = new_apple();    // advanced until growlen returns to 0.
					snake->grow = GROWLEN;  // See `move_snake()`
					score ++; 		// increment score
				}
				else
				{
					// If it wasn't an apple, then it must have been the tail.
					fatal("CRASHED INTO TAIL");
				}
			};
			// Because we moved and drew the snake into the frame buffer,
			// we must indicate to the interrupt that it's time to flush the
			// buffer to the screen and also reset the gamespeed counter.
			drawflag = true;
			gamespeed = GAMESPEED;
		}
	}
	crlf();
	quit();
}
