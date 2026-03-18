#include "audio.h"

AudioManager gAudio;

void initAudio() {
#ifdef USE_SDL_MIXER
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "[Audio] Mix_OpenAudio failed: %s\n", Mix_GetError());
        return;
    }
    gAudio.initialized = true;
#endif
}

void loadSFX(const std::string& id, const char* path) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    Mix_Chunk* chunk = Mix_LoadWAV(path);
    if (!chunk) {
        fprintf(stderr, "[Audio] Cannot load SFX '%s': %s\n", path, Mix_GetError());
        return;
    }
    gAudio.sfx[id] = chunk;
#else
    (void)id; (void)path;
#endif
}

void loadMusic(const std::string& id, const char* path) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    Mix_Music* mus = Mix_LoadMUS(path);
    if (!mus) {
        fprintf(stderr, "[Audio] Cannot load music '%s': %s\n", path, Mix_GetError());
        return;
    }
    gAudio.music[id] = mus;
#else
    (void)id; (void)path;
#endif
}

void playSFX(const std::string& id) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    auto it = gAudio.sfx.find(id);
    if (it == gAudio.sfx.end()) return;
    Mix_PlayChannel(-1, it->second, 0);
#else
    (void)id;
#endif
}

void playMusic(const std::string& id, bool loop) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    if (gAudio.currentMusic == id) return;
    auto it = gAudio.music.find(id);
    if (it == gAudio.music.end()) return;
    Mix_PlayMusic(it->second, loop ? -1 : 1);
    gAudio.currentMusic = id;
#else
    (void)id; (void)loop;
#endif
}

void stopMusic() {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    Mix_HaltMusic();
    gAudio.currentMusic = "";
#endif
}

void setAudioVolume(float v) {
    gAudio.masterVolume = v;
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    int vol = (int)(v * MIX_MAX_VOLUME);
    Mix_Volume(-1, vol);
    Mix_VolumeMusic(vol);
#endif
}

void cleanupAudio() {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    for (auto& [k, c] : gAudio.sfx)   { if (c) Mix_FreeChunk(c); }
    for (auto& [k, m] : gAudio.music) { if (m) Mix_FreeMusic(m); }
    gAudio.sfx.clear();
    gAudio.music.clear();
    Mix_CloseAudio();
    gAudio.initialized = false;
#endif
}
