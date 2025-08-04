<!-- vim: set ft=markdown tw=80 cc=80: -->
# 6502-Retro! Snake Game

This version of Snake differs from the versions I have made for the Nabu and the
Z80-Retro! in that it uses Multicolor mode instead of Graphics Mode 1 or hybrid
Graphics Mode 2 / 1.

## Build Instructions

The following dependencies must be in your path.

- python
- make
- cc65

Type `make` to build.

Copy the `build/snake.com` over to the SDCARD and run it.

## Controls

There is currently no on screen text rendered by the game.  All feedback from
the game will appear on the serial terminal.

After launching the game, it will start incrementing the random seed generator
until you press **[SPACE]** to begin the game.

Control the snake with the `WASD` keys.  Eat the green apples until you crash
into the walls or your own tail.

Have fun!
