// In src/io/sound.c

#include <SDL_mixer.h>
#include <stdio.h>
#include "sound.h"

// An array to hold sound chunks.
static Mix_Chunk* sounds[SOUND_MAX] = {NULL};

// A corresponding array of sound file paths.
static const char* sound_files[SOUND_MAX] = {
    "sounds/ufo_highpitch.wav",
    "sounds/shoot.wav",
    "sounds/explosion.wav",
    "sounds/invaderkilled.wav",
    "sounds/fleet_1.wav",
    "sounds/fleet_2.wav",
    "sounds/fleet_3.wav",
    "sounds/fleet_4.wav",
    "sounds/ufo_highpitch.wav" // UFO Hit can reuse the UFO sound
};

bool sound_init(void) {
    // Initialize SDL_mixer. 44100 Hz, 16-bit audio, 2 channels (stereo), 2048 chunk size.
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "SDL_mixer could not initialize! Mix_Error: %s\n", Mix_GetError());
        return false;
    }

    // Load each sound file.
    for (int i = 0; i < SOUND_MAX; i++) {
        sounds[i] = Mix_LoadWAV(sound_files[i]);
        if (sounds[i] == NULL) {
            fprintf(stderr, "Warning: Failed to load sound effect: %s\n", sound_files[i]);
            // Continue loading other sounds rather than failing completely
        }
    }

    return true;
}

void sound_play(SoundID id) {
    // check if the ID is valid.
    if (id >= SOUND_MAX) {
        return;
    }

    // Check if the sound for this ID was successfully loaded before trying to play it.
    if (sounds[id] != NULL) {
        Mix_PlayChannel(-1, sounds[id], 0);
    } else {
        // Sound is not loaded (it failed in sound_init), print a debug message instead.
        printf("DEBUG: Sound not loaded for ID %d (%s)\n", id, sound_files[id]);
    }
}

void sound_cleanup(void) {
    // Free each loaded sound chunk.
    for (int i = 0; i < SOUND_MAX; i++) {
        if (sounds[i] != NULL) {
            Mix_FreeChunk(sounds[i]);
        }
    }

    // Quit SDL_mixer.
    Mix_Quit();
    Mix_CloseAudio();
}