#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "6502retro_lib.h"
#include "vdp.h"
#include "bios.h"

#define BUFFERLEN 0x800
#define GROWLEN 5

enum e_dir {
	NORTH,
	EAST,
	SOUTH,
	WEST
};

typedef struct {
	int8_t x;
	int8_t y;
} Vec2;

typedef struct {
	Vec2 *head;
	Vec2 *tail;
	uint8_t grow;

} Snake;

Vec2 buffer[BUFFERLEN] = {0};	// 8k buffer to mess about with

void crlf()
{
	bios_puts("\r\n");
}

void quit()
{
	reset_interrupt();
	bios_wboot();
}

void fatal(char *msg)
{
	bios_puts(msg);
	crlf();
	quit();
}

void interrupt()
{
	if (drawflag)
	{
		vdp_flush(&FRAMEBUF);
		drawflag = false;
	}
}


void delay(uint8_t t)
{
	do {
		vdp_wait();
	} while (--t > 0);
}

Vec2 new_apple()
{
	bool taken = true;
	Vec2 a;
	while (taken)
	{
		a.x = (rand() & 0xFF) % 64;
		a.y = (rand() & 0xFF) % 48;
		if (!vdp_plot_xy(a.x,a.y,VDP_LIGHT_GREEN))
			taken = false;
	}
	return a;
}


Snake *new_snake()
{
	Snake *s;
	Vec2 v;
	uint8_t i;
	memset(&buffer, 0, BUFFERLEN);

	for (i=0; i<5; ++i)
	{
		v.x = 27+i;
		v.y = 24;
		buffer[i] = v;
	}
	s->head= &buffer[i-1];
	s->tail= &buffer[0];
	s->grow = 0;
	return s;
}

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

	if ((newseg.x > 63)
		|| (newseg.x < 0)
		|| (newseg.y > 47)
		|| (newseg.y < 0)
	)
	{
		fatal("CRASHED INTO WALL");
	}

	if (s->head < &buffer[BUFFERLEN]-2)
		s->head++;
	else
		s->head = &buffer[0];

	if (s->grow == 0)
	{
		if (s->tail < &buffer[BUFFERLEN]-2)
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

bool draw_snake(Snake *s)
{
	vdp_plot_xy(s->tail->x, s->tail->y, VDP_BLACK);
	return vdp_plot_xy(s->head->x, s->head->y, VDP_DARK_RED);
}

void main()
{
	size_t seed = 0;
	Vec2 apple;
	Snake *snake;
	uint8_t dir, k;
	uint8_t counter = 4;
	uint8_t running = false;
	vdp_reset();
	vdp_colorize(VDP_BLACK);
	set_interrupt(&interrupt);
	crlf();

	bios_puts("Press SPACE to play...\r\n");
	while (bios_const() != ' ') {
		++seed;
	}
	running = true;
	srand(seed);
	snake = new_snake();
	dir = EAST;
	draw_snake(snake);
	apple = new_apple();

	while (running)
	{
		delay(1);
		--counter;
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
		if (counter == 0)
		{
			move_snake(snake, dir);
			if (draw_snake(snake))
			{
				if (
					(snake->head->x == apple.x) &&
					(snake->head->y == apple.y)
				)
				{
					apple = new_apple();
					snake->grow = GROWLEN;
				}
				else
				{
					fatal("CRASHED INTO TAIL");
				}
			};
			drawflag = true;
			counter = 4;
		}
	}
	quit();
}
