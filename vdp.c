#include <stdint.h>
#include <string.h>
#include "vdp.h"
#include "6502retro_lib.h"

extern uint16_t i;
extern uint16_t j;

char *framebuf = &FRAMEBUF;


uint8_t vdp_plot_xy(uint8_t x, uint8_t y, uint8_t c) {
    static uint8_t dot = 0;
    static uint8_t old = 0;
    static char pix = 0;
    static uint16_t addr = 0;
    char collide = 0;
    rambankreg = bufferbank;
    //addr = (8 * (x / 2)) + (y % 8) + (256 * (y / 8));
    addr = vdp_xy_to_offset(x<<8|y);
    dot = framebuf[addr];

    if (x & 1) // Odd columns
    {
        // -X
        if ( ((dot & 0x0F) > VDP_BLACK) && (c > VDP_BLACK))
        {
            collide = 1;
        }
        pix = (dot & 0xF0) | c;
    }
    else
    {
        // X-
        if ( (c > VDP_BLACK) && (dot >> 4) > VDP_BLACK)
        {
            collide = 1;
        }
        pix = (dot & 0x0F) | (c<<4);
    }
    framebuf[addr] = pix;
    rambankreg = 0;
    return collide;
}

// Set all pixels to black
void vdp_colorize(uint8_t c) {
    uint8_t b = c<<4 | c;
    rambankreg = bufferbank;
    memset(framebuf, b, 0x600);
    rambankreg = 0;
}

// vim: ts=4 sw=4 et:
