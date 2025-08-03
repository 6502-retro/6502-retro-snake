#ifndef RETRO_LIB_H_
#define RETRO_LIB_H_
#include <stdint.h>

#define rambankreg *(char*)0xBF00
#define bufferbank 1

extern void vdp_reset();
extern void __fastcall__ vdp_set_read_addr(uint16_t);
extern void __fastcall__ vdp_set_write_addr(uint16_t);
extern uint16_t __fastcall__ vdp_xy_to_offset(uint16_t);
extern void vdp_clear_pattern_table();
extern void vdp_wait();
extern void set_interrupt(void*);
extern void reset_interrupt();
extern void sn_play_note();

#endif //RETRO_LIB_H_
