#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "vdp.h"
#include "6502retro_lib.h"

extern uint16_t i;
extern uint16_t j;
bool drawflag;
char *framebuf = &FRAMEBUF;


/* VDP Plot XY (Multicolor Mode)
* Returns boolean if the colour being plotted is not black or transparent
* and clashes with an existing colour in the frame buffer
*/
bool vdp_plot_xy(uint8_t x, uint8_t y, uint8_t c) {
    static uint8_t dot = 0;         // Existing double-pixel in frame buffer
    static char pix = 0;            // New value of double-pixel in frame buffer
    static uint16_t addr = 0;       // Address offset into the framebuffer (index)
    char collide = false;           // True if collision detected
    rambankreg = bufferbank;        // We set the rambank to the bank holding the frame buffer
    addr = vdp_xy_to_offset(x<<8|y);// Get the address offset into the frame buffer from XY as 16bit value
    dot = framebuf[addr];           // Existing double-pixel value

    /*
    * Depending on if we are dealing with the left or right (even or odd) pixel in the double pixel.
    * We mask and shift so we can find a collision and set the new double-pixel
    * We don't consider BLACK or TRANSPARENT as colours that would result in a collision.
    */
    if (x & 1) // Odd columns
    {
        // -X
        if ( ((dot & 0x0F) > VDP_BLACK) && (c > VDP_BLACK))
        {
            collide = true;
        }
        pix = (dot & 0xF0) | c;
    }
    else
    {
        // X-
        if ( (c > VDP_BLACK) && (dot >> 4) > VDP_BLACK)
        {
            collide = true;
        }
        pix = (dot & 0x0F) | (c<<4);
    }
    framebuf[addr] = pix;
    rambankreg = 0;                 // reset the ram bank to 0
    return collide;
}

// Set all pixels to color passed in.
void vdp_colorize(uint8_t c) {
    uint8_t b = c<<4 | c;           // The colour is 0x?? for both left and right pixels in the double-pixel
    rambankreg = bufferbank;
    memset(framebuf, b, 0x600);
    rambankreg = 0;
}

// vim: ts=4 sw=4 et:
