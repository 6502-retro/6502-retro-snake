; vim: set ft=asm_ca65 ts=4 sw=4 et:
.include "const.inc"
.include "app.inc"
.include "macro.inc"
.include "io.inc"

.export _vdp_reset, _vdp_set_write_addr, _vdp_set_read_addr, _vdp_wait
.export _vdp_flush, _vdp_xy_to_offset, _vdp_clear_pattern_table
.export _set_interrupt, _reset_interrupt
.export _sn_play_note, _sn_play_noise

.export _notectr := notectr

.autoimport

FIRST   = %10000000
SECOND  = %00000000
CHAN_1  = %00000000
CHAN_2  = %00100000
CHAN_3  = %01000000
CHAN_N  = %01100000
TONE    = %00000000
VOL     = %00010000
VOL_OFF = %00001111
VOL_MAX = %00000000

FRAMEBUF = $C000
FRAMEBUFBANK = 1


.zeropage

v1: .word 0
v2: .word 0
v3: .word 0


.macro set_framebuffer_bank
    lda #FRAMEBUFBANK
    sta RAMBANKREG
.endmacro

.macro reset_rambank
    stz RAMBANKREG
.endmacro

.code

_sn_play_note:
    lda #$07
    ora #(FIRST|CHAN_1|TONE)
    jsr bios_sn_send
    lda #$04
    ora #(SECOND|CHAN_1|TONE)
    jsr bios_sn_send
    lda #(FIRST|CHAN_1|VOL|$04)
    jsr bios_sn_send
    rts

_sn_play_noise:
    lda #4
    ora #(FIRST|CHAN_N)
    jsr bios_sn_send
    lda #(FIRST|CHAN_N|VOL|VOL_MAX)
    jsr bios_sn_send
    rts

; reset vdp into MC mode and set up the nametable with 6 banks each with 4 rows of the same values.
; 0-31    (x4)
; 32-63   (x4)
; 64-95   (x4)
; 96-127  (x4)
; 128-159 (x4)
; 160-192 (x4)
; = 768 nametable positions.
_vdp_reset:
    set_framebuffer_bank
    jsr vdp_clear_vram
    jsr vdp_init_mc
    ; init name table
    lda #<VDP_NAME
    ldx #>VDP_NAME
    jsr _vdp_set_write_addr

    lda #6
    sta v1+0
    lda #0
    sta v2+0
@lpsection:
    lda #4
    sta v1+1
@lpline:
    ldx #32
    lda v2+0
@lpcol:
    sta VDP_RAM
    inc
    dex
    bne @lpcol

    dec v1+1      ; lines
    bne @lpline
@lpsection_lp:
    lda #32
    clc
    adc v2+0
    sta v2+0

    dec v1+0
    bne @lpsection
    reset_rambank
    rts

; The system interrupt handler calls a routine that jmps to a
; vector defined at `bios_userieq_vec`.; By default this vector
; points to a function that simply returns.  Here we save the original
; vector so it can be restored later and update it with a pointer
; to our own handler.
; INPUT: XA = Pointer to custom user interrupt routine
; OUTPUT: VOID
_set_interrupt:
    sta v1+0
    stx v1+1
    lda bios_userirq_vec + 0
    sta old_irq_vec + 0
    lda bios_userirq_vec + 1
    sta old_irq_vec + 1

    lda v1+0
    sta bios_userirq_vec + 0
    lda v1+1
    sta bios_userirq_vec + 1
    rts

; restore the original userirq_vector.
; INPUT: VOID
; OUTPUT: VOID
_reset_interrupt:
    lda old_irq_vec + 0
    sta bios_userirq_vec + 0
    lda old_irq_vec + 1
    sta bios_userirq_vec + 1
    rts

; Zero out all 16KB of VRAM.
; INPUT: VOID
; OUTPUT: VOID
vdp_clear_vram:
    set_framebuffer_bank
    lda #0                  ; A is low byte of vram write address
    ldx #0                  ; X is high byte of vram write address
    jsr _vdp_set_write_addr ; set the starting address to zero.
    lda #0                  ; A has the value being written to VRAM
    ldy #0                  ; Y is the byte counter
    ldx #$3F                ; X is the page counter
:   sta VDP_RAM             ; save A into vram
    slow
    iny                     ; increment Y and loop until a whole page is written
    bne :-
    dex                     ; decement page counter and loop until all 0x3F
    bne :-                  ; pages are written.
    reset_rambank
    rts

; Copy 0x600 bytes of data from the framebuffer pointed to by XA
; to the VDP Pattern Generator Table.  This is how a flush works in
; multicolor mode.
; INPUT: XA = pointer to framebuffer
; OUTPUT: VOID
_vdp_flush:
    sta v1+0
    stx v1+1
    set_framebuffer_bank

    lda #<VDP_PATTERN
    ldx #>VDP_PATTERN
    jsr _vdp_set_write_addr
    ldx #$06
    ldy #0
@lp:
    lda (v1),y
    sta VDP_RAM
    iny
    bne @lp
    inc v1+1
    dex
    bne @lp
    reset_rambank
    rts

; Sets the VDP internal VRAM pointer for writing.
; INPUT: A is the low byte of the VRAM address.
;        X is the high byte of the VRAM address.
; OUTPUT: VOID
_vdp_set_write_addr:
    sta VDP_REG             ; As per the TI Programmers Guide.
    fast
    txa
    ora #$40
    sta VDP_REG
    fast
    rts

; Sets the VDP internal VRAM pointer for reading.
; INPUT: A is the low byte of the VRAM address.
;        X is the high byte of the VRAM address.
; OUTPUT: VOID
_vdp_set_read_addr:
    sta VDP_REG             ; As per the TI Programmers Guide.
    fast
    stx VDP_REG
    fast
    rts

; Sets all the doublepixels in multicolor mode to black.
; INPUT: VOID
; OUTPUT: VOID
_vdp_clear_pattern_table:
    set_framebuffer_bank
    lda #<VDP_PATTERN
    ldx #>VDP_PATTERN
    jsr _vdp_set_write_addr
    lda #$11
    ldy #$00
    ldx #$06
:   sta VDP_RAM
    iny
    bne :-
    dex
    bne :-
    reset_rambank
    rts

; Calculate the framebuffer index for a pixel given in 64x32 coordinate space
; addr = (8 * (x / 2)) + (y % 8) + (256 * (y / 8));
; INPUT: XA = X = X coordinate, A = Y coordinate
; OUTPUT: XA = 16bit offset into framebuffer
_vdp_xy_to_offset:
    tay
    div8
    sta v2+1  ; Y / 8 (saved into high byte which means *256)
    tya
    and #$07    ; Y mod 8
    sta v3+0
    txa
    lsr         ; X / 2
    asl
    asl
    asl         ; X * 8
    clc
    adc v3+0    ; + y % 8
    sta v2+0
    ldx v2+1
    rts

; initialize vdp in Multicolour mode (64x48)
vdp_init_mc:
    lda #<mc_regs
    ldx #>mc_regs
    jmp _init_regs

; Waits for the VDP Interrupt to fire.
_vdp_wait:
    bit VDP_SYNC            ; vdp_sync is set by the interrupt handler to 0x80
    bpl _vdp_wait
    stz VDP_SYNC            ; an interrupt was received so set the vdp_sync var
    rts                     ; to zero before exiting.

; Copy a table of register values to the VDP.  The table is arranged in order
; from REG-0 to REG-7.  Each value is set to the register of its position in the
; table.
; INPUT: A is the low byte of register table address.
;        X is the high byte of register table address.
; OUTPUT: VOID
_init_regs:
    sta v3                ; set up a pointer to the register table.
    stx v3+1
    ldy #0                  ; Y is the offset in the register table.
:   lda (v3),y            ; load the first byte
    sta VDP_REG             ; save to VRAM
    fast
    tya                     ; use the pointer offset to set the VDP register
    ora #$80                ; As per the TI Programmers manual.
    sta VDP_REG
    fast
    iny
    cpy #8                  ; there are 8 registers altogether 0-7
    bne :-                  ; loop until complete.
    rts

old_irq_vec: .word 0        ; Storage for original interrupt vector

.rodata
; These are the registers for the multicolour mode
mc_regs:
    .byte $00               ; M3 = 0
    .byte (VDP_16K|VDP_BLANK_SCREEN|VDP_INTERRUPT_ENABLE|VDP_M2)   ; M1=0, M2=1
    .byte $05               ; Address table = 0x1400
    .byte $00               ; colour table not used
    .byte $01               ; Pattern table at 0x800
    .byte $20               ; Sprite attribute table at 0x1000
    .byte $00               ; Sprite pattern table at 0x0000
    .byte VDP_DARK_YELLOW   ; backdrop colour
