# CS467_Emulator - 8080 Space Invaders Emulator (Graphics)

## Overview

This directory contains source files used in emulating and rendering the graphical elements

## Background and Specifications

*   Space Invaders has a resolution of 224 pixels (width) by 256 pixels (height).
*   Graphics are drawn in memory (VRAM) from 0x2400 to 0x3FFF, which the video controller reads and renders onto the hardware display[^1][^2].
*   Each bit represents a pixel, and is rendered left to right, and then rotated 90 degrees counterclockwise (vertical retrace) to give the user a 224 (wide) by 256 (high) screen[^3].

## Technologies Used

The Simple DirectMedia Layer (SDL) library is used to handle screen emulation. SDL will emulate the game image window.

[^1]: https://web.archive.org/web/20240718053956/http://emulator101.com/
[^2]: https://web.archive.org/web/20240119084322/https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#screen-geometry
[^3]: https://www.walkofmind.com/programming/side/hardware.htm#:~:text=The%20physical%20video%20screen%2C%20however,a%20%22standard%22%20screen%20layout.

## Setup

1.  Run `sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev` to install the SDL development libraries.
2.  Compile the graphics program using `gcc graphics.c -o graphics -lSDL2` (SDL2 library link).
3.  Run using `.\graphics` (or other executable name).
4.  Quit by closing the graphics window (or Ctrl+C in terminal).

Run `sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev` to install the SDL development libraries.

## Resources
*   [SDL2 Wiki](https://wiki.libsdl.org/SDL2/FrontPage)