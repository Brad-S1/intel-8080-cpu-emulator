// In src/io/sound.h

#ifndef SOUND_H
#define SOUND_H

#include <stdbool.h>

typedef enum {
    SOUND_UFO,
    SOUND_SHOT,
    SOUND_PLAYER_DIE,
    SOUND_INVADER_DIE,
    SOUND_FLEET_1,
    SOUND_FLEET_2,
    SOUND_FLEET_3,
    SOUND_FLEET_4,
    SOUND_UFO_HIT,
    SOUND_MAX // A count of how many sounds we have
} SoundID;

// Initializes the SDL_mixer and loads all sound files.
// Returns true on success, false on failure.
bool sound_init(void);

// Plays the sound corresponding to the given ID.
void sound_play(SoundID id);

// Frees all loaded sound resources and shuts down the mixer.
void sound_cleanup(void);

#endif // SOUND_H