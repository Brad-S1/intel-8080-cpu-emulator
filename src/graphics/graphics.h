#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdbool.h>

// CPU header state (defined in cpu header)
struct State8080;  

// initializes sdl graphics (window, renderer, texture) 
bool graphics_init(void);

// cleans up sdl resources
void graphics_cleanup(void);

// render 8080 vram to screen
void graphics_draw(uint8_t* memory);

#endif  // GRAPHICS_H