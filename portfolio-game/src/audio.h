#pragma once
/**
 * audio.h — AudioManager stub
 * Zielony Pływak | Phase 0.4
 *
 * Full SDL2_mixer implementation comes in Phase 6.
 * This stub defines the interface so all call sites can be wired up now.
 * All functions are no-ops until USE_SDL_MIXER is enabled in the Makefile.
 */

#include <string>
#include <map>
#include <cstdio>

#ifdef USE_SDL_MIXER
  #include <SDL2/SDL_mixer.h>
#endif

struct AudioManager {
#ifdef USE_SDL_MIXER
    std::map<std::string, Mix_Chunk*>  sfx;
    std::map<std::string, Mix_Music*>  music;
    std::map<std::string, int>         musicChannels;
#endif
    float       masterVolume   = 1.0f;
    std::string currentMusic;
    bool        initialized    = false;
};

// Global audio manager (defined in audio.cpp)
extern AudioManager gAudio;

// ── API ──────────────────────────────────────────────────────────────────────

void initAudio();
void loadSFX  (const std::string& id, const char* path);
void loadMusic(const std::string& id, const char* path);
void playSFX  (const std::string& id);
void playMusic(const std::string& id, bool loop = true);
void stopMusic();
void setAudioVolume(float v);   // 0.0–1.0
void cleanupAudio();
