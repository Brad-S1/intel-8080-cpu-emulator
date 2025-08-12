// tester file to validate graphics function
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include "graphics.h"
#include "cpu.h" // defines State8080 and ConditionCodes structs (from emulator_shell)

// compile and run
// gcc graphics_tester.c graphics.c -o graphics_tester $(sdl2-config --cflags --libs)
// ./graphics_tester

// draws a test pattern into VRAM memory (horizontal bars)
// input parameters: CPU state (vram) and bool toggle (draw mode)
void draw_pattern(uint8_t* memory, bool toggle)
{
  memset(memory, 0, MEMORY_SIZE); // clear all memory

  // write VRAM with a test pattern (horizontal bars on screen)
  // draw vertical bars in VRAM columns (one byte every 32 = 1 full screen column)
  for (int col = 0; col < 32; col++)
  {
    // toggle = 0: set even columns to white (on) and odd columns to black (off)
    // toggle = 1: set odd columns to white (on) and even columns to black (off)
    uint8_t pixel_value = ((col % 2 == toggle) ? 0xFF : 0x00);

    for (int row = 0; row < 224; row++)
    {
      int index = row * 32 + col; // calculate byte index in VRAM relative to VRAM start address
      memory[0x2400 + index] = pixel_value;
    }
  }
}

int main(int argc, char *argv[])
{

  // initialize 8080 CPU state
  State8080 *state = (State8080 *)calloc(1, sizeof(State8080));
  if (state == NULL)
  {
    fprintf(stderr, "Failed to allocate CPU State\n");
    return 1;
  }

  // Initialize memory
  state->memory = (uint8_t *)calloc(MEMORY_SIZE, sizeof(uint8_t));
  if (state->memory == NULL)
  {
    fprintf(stderr, "Failed to allocate memory\n");
    free(state);
    return 1;
  }

  // Initialize graphics
  if (!graphics_init())
  {
    fprintf(stderr, "Failed to initialize graphics\n");
    SDL_Quit();
    return 1;
  }

  // emulation loop
  bool quit = false;
  bool toggle = false; // toggle test image a and b
  int frame_counter = 0;

  while (!quit)
  {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0)
    {
      // printf("Event type: %d\n", e.type);
      if (e.type == SDL_QUIT)
      {
        quit = true;
      }
    }

    // alternate test images using toggle every 30 frames
    if (frame_counter % 30 == 0)
    {
      printf("drawing pattern %s\n", toggle ? "a" : "b");
      draw_pattern(state->memory, toggle);
      toggle = !toggle;
    }

    graphics_draw(state->memory);
    SDL_Delay(16);
    frame_counter++;
  }

  printf("exiting\n");
  graphics_cleanup();
  SDL_Quit();

  return 0;

}
