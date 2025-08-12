// Testing of SDL2 graphics
//
// compile: gcc graphics.c -o graphics -lSDL2
// 
#include <SDL2/SDL.h>
#include <stdio.h>

// #define SCREEN_WIDTH    224
// #define SCREEN_HEIGHT   256

const int SCREEN_WIDTH = 224;
const int SCREEN_HEIGHT = 256;

// emlated VRAM (1 bit = 1 pixel)
#define VRAM_SIZE 0x1C00
uint8_t vram[VRAM_SIZE];

// buffer used to update (32-bit argb)
uint32_t* pixel_buffer = NULL;

// helper function to convert vram buffer bits to 32-bit SDL pixels
void update_pixel_buffer_from_vram() {
    // argb format colors
    const uint32_t color_on = 0xFF00FF00;  // green
    const uint32_t color_off = 0xFF000000;  // black

    // iterate over each pixel of the emulated screen
    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
        for (int x = 0; x < SCREEN_WIDTH; ++x) {
            // convert (x,y) to memory byte and bit index
            int pixel_index = (y * SCREEN_WIDTH) + x;   // convert coordinate to index
            int vram_byte_index = pixel_index / 8;      // find byte index of target pixel bit
            int bit_offset = pixel_index % 8;           // bit index (0-7)

            // check if the bit for the current pixel is set in VRAM
            if (vram[vram_byte_index] & (1 << bit_offset)) {
                pixel_buffer[pixel_index] = color_on;
            } else {
                pixel_buffer[pixel_index] = color_off;
            }
        }
    }
}

// helper function to set pixel value in VRAM
void set_pixel(int x, int y, int state) {
    // check if (x,y) are out of bounds
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) {
        return;
    }

    int pixel_index = (y * SCREEN_WIDTH) + x;
    int vram_byte_index = pixel_index / 8;
    int bit_offset = pixel_index % 8;

    if (state == 1) {
        // turn the bit on using bitwise OR
        vram[vram_byte_index] |= (1 << bit_offset);
    } else {
        // turn the bit off using bitwise AND with the inverse of the mask
        vram[vram_byte_index] &= ~(1 << bit_offset);
    }
}

int main(int argc, char* argv[]) {
    // SDL (Window, Renderer, Texture) pointers
    SDL_Window* window = NULL;          // graphics window
    SDL_Renderer* renderer = NULL;
    SDL_Texture* screen_texture = NULL;  // framebuffer
    
    // initialize SDL library with video subsystem flag
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // if initialization fails, print the error and exit
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // create Window
    window = SDL_CreateWindow(
        "SDL Window Title",          
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        // if window creation fails, print the error and exit
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // create Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // create Texture
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (screen_texture == NULL) {
        printf("Texture could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // create pixel buffer
    pixel_buffer = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (pixel_buffer == NULL) {
        printf("Could not allocate pixel buffer!.\n");
        SDL_DestroyTexture(screen_texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // set vram bytes to 0
    memset(vram, 0, VRAM_SIZE);

    // set pixel drawing
    // used AI here to create the test pixel drawing code
    // Define a simple 11x8 invader sprite
    const uint8_t invader_sprite[8][11] = {
        {0,0,1,0,0,0,0,0,1,0,0},
        {0,0,0,1,0,0,0,1,0,0,0},
        {0,0,1,1,1,1,1,1,1,0,0},
        {0,1,1,0,1,1,1,0,1,1,0},
        {1,1,1,1,1,1,1,1,1,1,1},
        {1,0,1,1,1,1,1,1,1,0,1},
        {1,0,1,0,0,0,0,0,1,0,1},
        {0,0,0,1,1,0,1,1,0,0,0}
    };

    // Draw the invader sprite onto the VRAM, scaled up to be large
    int scale = 8;
    int start_x = (SCREEN_WIDTH - (11 * scale)) / 2; // Center horizontally
    int start_y = (SCREEN_HEIGHT - (8 * scale)) / 2; // Center vertically

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 11; x++) {
            if (invader_sprite[y][x] == 1) {
                // Draw a scaled block for each "on" pixel in the sprite
                for (int sy = 0; sy < scale; sy++) {
                    for (int sx = 0; sx < scale; sx++) {
                        set_pixel(start_x + (x * scale) + sx, start_y + (y * scale) + sy, 1);
                    }
                }
            }
        }
    }


    // main loop
    int quit = 0;
    SDL_Event e;  // event handling

    while (!quit) {
        // handle events on queue
        while (SDL_PollEvent(&e) != 0) {
            // quit event handling
            if (e.type == SDL_QUIT) {
                quit = 1;
            }


        }

        // drawing logic

        // TODO: CPU emulation and modification to vram buffer
        
        // translate the 1-bit VRAM data into 32-bit pixel buffer
        update_pixel_buffer_from_vram();

        // update the SDL texture with the data from pixel buffer.
        SDL_UpdateTexture(screen_texture, NULL, pixel_buffer, SCREEN_WIDTH * sizeof(uint32_t));

        // clear renderer, copy texture
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);

        // present render
        SDL_RenderPresent(renderer);

    }

    // cleanup (exit and free)
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();  // clean up SDL subsystems
    
    return 0;
}
