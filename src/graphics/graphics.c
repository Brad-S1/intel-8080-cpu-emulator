// Space Invaders Emulator - Graphics Library
//
// Resources Consulted:
// https://walkofmind.com/programming/side/hardware.htm
// https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#:~:text=The%20raster%20resolution%20is%20256x224,=%207168%20(7K)%20bytes.
//
// compile: gcc graphics.c -o graphics -lSDL2
// ./graphics

#include <SDL.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h> 

#include "cpu.h"

// SDL (Window, Renderer, Texture) pointers
static SDL_Window *window = NULL;     // graphics window
static SDL_Renderer *renderer = NULL; // drawer
static SDL_Texture *texture = NULL;   // framebuffer

// screen size constants
static const int SCREEN_WIDTH = 224;
static const int SCREEN_HEIGHT = 256;
static const int WINDOW_SCALE = 5; // scaling factor for screen dimensions

// vram constants
static const int VRAM_START = 0x2400; // space invaders vram starts at memory address 0x2400
static const int VRAM_SIZE = 7168;    // (224 pixels * 256 pixels) / 8 bits = 7168 bytes

// initializes graphics (vidoe subsystem)
// returns true if initalization successful
bool graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Could not initialize SDL Video: %s\n", SDL_GetError());
        return false;
    }

    // create Window
    window = SDL_CreateWindow(
        "Space Invaders",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * WINDOW_SCALE,
        SCREEN_HEIGHT * WINDOW_SCALE,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return false;
    }

    // create Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    // Renderer settings
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");                 // scaling algorithm
    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT); // renderer handles resized GUI

    // create Texture
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!texture)
    {
        fprintf(stderr, "Could not create texture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return false;
    }

    return true;
}

// free video subsystem allocated memory
void graphics_cleanup(void)
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

// draw graphics based on memory (vram)
void graphics_draw(uint8_t *memory)
{
    uint32_t *pixels = NULL; // pixel buffer
    int pitch = 0;           // LockTexture computes row pitch

    // SDL library function to lock the texture and point to pixel buffer
    if (SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch) != 0)
    {
        fprintf(stderr, "Could not lock texture: %s\n", SDL_GetError());
    }

    // pointer to memory parameter
    unsigned char *vram = &memory[VRAM_START]; // vram starts at 0x2400

    // iterate over vram
    for (int i = 0; i < VRAM_SIZE; i++)
    {
        unsigned char byte = vram[i];

        // vram (x,y) = (256,224); screen(x,y) = (224,256)
        // bytes run right horizontally (x) then down vertically (y) in 8 bit chunks
        // determine the starting (x,y) coordinate of current byte
        int x = (i % 32) * 8; // 32 bytes per row
        int y = i / 32;

        // iterate through each bit/pixel in current byte
        for (int bit = 0; bit < 8; bit++)
        {
            // shift current bit to LSB position and set black or white
            uint32_t color = ((byte >> bit) & 1) ? 0xFFFFFFFF : 0xFF000000;

            // 90 degree rotation counterclockwise
            int x_rotated = y;
            int y_rotated = 255 - (x + bit); // 0 is top left corner in SDL

            // check boundaries and set pixel color
            if (x_rotated < SCREEN_WIDTH && y_rotated < SCREEN_HEIGHT)
            {
                // convert coordinates to pixel buffer index position
                pixels[y_rotated * SCREEN_WIDTH + x_rotated] = color;
            }
        }
    }

        // unlock and render
        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    
}
