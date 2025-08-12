// In src/io/input.c

// inspired by Simple DirectMedia Layer (SDL) Wiki
// https://wiki.libsdl.org/SDL2/FrontPage
// also inspired by:
// https://github.com/tommojphillips/Space-Invaders/blob/master/src/input.c

#include <SDL2/SDL.h>
#include <stdio.h>
#include "input.h"

// The init function for maintaining a consistent API.
int io_init(void) {
    // Does nothing now. Graphics module handles SDL initialization.
    return 0;
}

// The cleanup is also handled by the graphics module.
void io_cleanup(void) {
    // Does nothing.
}

int io_handle_input(MachineState* machine) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return 1; // Signal to quit
        }

        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            int is_pressed = (event.type == SDL_KEYDOWN);
            switch (event.key.keysym.sym) {
                case SDLK_c:     // Coin
                    if (is_pressed) machine->port1 |= 0x01; else machine->port1 &= ~0x01;
                    break;
                case SDLK_1:     // P1 Start
                    if (is_pressed) machine->port1 |= 0x04; else machine->port1 &= ~0x04;
                    break;
                case SDLK_SPACE: // P1 Shoot
                    if (is_pressed) machine->port1 |= 0x10; else machine->port1 &= ~0x10;
                    break;
                case SDLK_LEFT:  // P1 Left
                    if (is_pressed) machine->port1 |= 0x20; else machine->port1 &= ~0x20;
                    break;
                case SDLK_RIGHT: // P1 Right
                    if (is_pressed) machine->port1 |= 0x40; else machine->port1 &= ~0x40;
                    break;
                
                // Add Player 2 controls
                case SDLK_2:     // P2 Start
                    if (is_pressed) machine->port1 |= 0x02; else machine->port1 &= ~0x02;
                    break;
                case SDLK_q:     // P2 Left (example)
                    if (is_pressed) machine->port2 |= 0x20; else machine->port2 &= ~0x20;
                    break;
                case SDLK_w:     // P2 Right (example)
                    if (is_pressed) machine->port2 |= 0x40; else machine->port2 &= ~0x40;
                    break;
                case SDLK_e:     // P2 Fire (example)
                    if (is_pressed) machine->port2 |= 0x10; else machine->port2 &= ~0x10;
                    break;
            }
        }
    }
    return 0;
}
