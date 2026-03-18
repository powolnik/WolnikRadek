/**
 * audio.cpp — AudioManager full implementation
 * Zielony Pływak | Phase 6
 *
 * Uses SDL2_mixer when compiled with -DUSE_SDL_MIXER.
 * All functions compile to no-ops without that flag.
 *
 * Music "layering": SDL2_mixer supports only one Music stream at a time.
 * Each music track therefore contains all previous layers baked in
 * (i.e. layer2 = base + shallows mix, layer3 = base + shallows + meadow, etc.).
 * The composer must deliver pre-mixed OGG files for each milestone.
 */

#include "audio.h"
#include <cstdio>

AudioManager gAudio;

// ── Asset paths — swap in real files when composer delivers ──────────────────
namespace AudioPaths {
    // Music (OGG, pre-mixed cumulative layers)
    constexpr const char* MUSIC_BASE      = "assets/audio/music_ocean_base.ogg";
    constexpr const char* MUSIC_LAYER2    = "assets/audio/music_layer_shallows.ogg";
    constexpr const char* MUSIC_LAYER3    = "assets/audio/music_layer_meadow.ogg";
    constexpr const char* MUSIC_LAYER4    = "assets/audio/music_layer_forest.ogg";
    constexpr const char* MUSIC_LAYER5    = "assets/audio/music_layer_hill.ogg";
    constexpr const char* MUSIC_ENDING    = "assets/audio/music_ending.ogg";
    // SFX — actions
    constexpr const char* SFX_PICKUP      = "assets/audio/sfx_pickup.wav";
    constexpr const char* SFX_RECYCLE     = "assets/audio/sfx_recycle.wav";
    constexpr const char* SFX_COMPOST     = "assets/audio/sfx_compost_squelch.wav";
    // SFX — footsteps
    constexpr const char* SFX_STEP_SAND   = "assets/audio/sfx_step_sand.wav";
    constexpr const char* SFX_STEP_WATER  = "assets/audio/sfx_step_water.wav";
    constexpr const char* SFX_STEP_GRASS  = "assets/audio/sfx_step_grass.wav";
    constexpr const char* SFX_STEP_STONE  = "assets/audio/sfx_step_stone.wav";
    constexpr const char* SFX_STEP_WOOD   = "assets/audio/sfx_step_wood.wav";
    // SFX — UI / interaction
    constexpr const char* SFX_ZONE_OPEN   = "assets/audio/sfx_zone_gate_open.wav";
    constexpr const char* SFX_DIALOGUE    = "assets/audio/sfx_dialogue_advance.wav";
    constexpr const char* SFX_ATLAS_UNLOCK= "assets/audio/sfx_atlas_unlock.wav";
    constexpr const char* SFX_TOOL_BORROW = "assets/audio/sfx_tool_borrow.wav";
    constexpr const char* SFX_TOOL_RETURN = "assets/audio/sfx_tool_return.wav";
    constexpr const char* SFX_MINDFULNESS = "assets/audio/sfx_mindfulness_ambience.wav";
    constexpr const char* SFX_DISMANTLE   = "assets/audio/sfx_dismantle.wav";
    constexpr const char* SFX_WIN         = "assets/audio/sfx_win_chime.wav";
    // SFX — animal calls
    constexpr const char* SFX_SEAGULL     = "assets/audio/sfx_animal_seagull.wav";
    constexpr const char* SFX_CRAB        = "assets/audio/sfx_animal_crab.wav";
    constexpr const char* SFX_TURTLE      = "assets/audio/sfx_animal_turtle.wav";
    constexpr const char* SFX_HARE        = "assets/audio/sfx_animal_hare.wav";
    constexpr const char* SFX_BEE         = "assets/audio/sfx_animal_bee.wav";
    constexpr const char* SFX_DEER        = "assets/audio/sfx_animal_deer.wav";
    constexpr const char* SFX_WOODPECKER  = "assets/audio/sfx_animal_woodpecker.wav";
    constexpr const char* SFX_EAGLE       = "assets/audio/sfx_animal_eagle.wav";
}

// ── Initialization ────────────────────────────────────────────────────────────

void initAudio() {
#ifdef USE_SDL_MIXER
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("[Audio] SDL_mixer init failed: %s\n", Mix_GetError());
        return;
    }
    Mix_AllocateChannels(32);
    gAudio.initialized = true;
    printf("[Audio] Initialized OK (32 channels)\n");

    // Pre-load all SFX
    loadSFX("pickup",       AudioPaths::SFX_PICKUP);
    loadSFX("recycle",      AudioPaths::SFX_RECYCLE);
    loadSFX("compost",      AudioPaths::SFX_COMPOST);
    loadSFX("step_sand",    AudioPaths::SFX_STEP_SAND);
    loadSFX("step_water",   AudioPaths::SFX_STEP_WATER);
    loadSFX("step_grass",   AudioPaths::SFX_STEP_GRASS);
    loadSFX("step_stone",   AudioPaths::SFX_STEP_STONE);
    loadSFX("step_wood",    AudioPaths::SFX_STEP_WOOD);
    loadSFX("zone_open",    AudioPaths::SFX_ZONE_OPEN);
    loadSFX("dialogue",     AudioPaths::SFX_DIALOGUE);
    loadSFX("atlas_unlock", AudioPaths::SFX_ATLAS_UNLOCK);
    loadSFX("tool_borrow",  AudioPaths::SFX_TOOL_BORROW);
    loadSFX("tool_return",  AudioPaths::SFX_TOOL_RETURN);
    loadSFX("mindfulness",  AudioPaths::SFX_MINDFULNESS);
    loadSFX("dismantle",    AudioPaths::SFX_DISMANTLE);
    loadSFX("win",          AudioPaths::SFX_WIN);
    loadSFX("seagull",      AudioPaths::SFX_SEAGULL);
    loadSFX("crab",         AudioPaths::SFX_CRAB);
    loadSFX("turtle",       AudioPaths::SFX_TURTLE);
    loadSFX("hare",         AudioPaths::SFX_HARE);
    loadSFX("bee",          AudioPaths::SFX_BEE);
    loadSFX("deer",         AudioPaths::SFX_DEER);
    loadSFX("woodpecker",   AudioPaths::SFX_WOODPECKER);
    loadSFX("eagle",        AudioPaths::SFX_EAGLE);

    // Pre-load all music
    loadMusic("base",    AudioPaths::MUSIC_BASE);
    loadMusic("layer2",  AudioPaths::MUSIC_LAYER2);
    loadMusic("layer3",  AudioPaths::MUSIC_LAYER3);
    loadMusic("layer4",  AudioPaths::MUSIC_LAYER4);
    loadMusic("layer5",  AudioPaths::MUSIC_LAYER5);
    loadMusic("ending",  AudioPaths::MUSIC_ENDING);
#else
    printf("[Audio] Compiled without USE_SDL_MIXER — audio disabled\n");
#endif
}

// ── Loading ───────────────────────────────────────────────────────────────────

void loadSFX(const std::string& id, const char* path) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    Mix_Chunk* c = Mix_LoadWAV(path);
    if (!c) {
        printf("[Audio] SFX not found: %s (%s)\n", path, Mix_GetError());
        return;
    }
    gAudio.sfx[id] = c;
#else
    (void)id; (void)path;
#endif
}

void loadMusic(const std::string& id, const char* path) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    Mix_Music* m = Mix_LoadMUS(path);
    if (!m) {
        printf("[Audio] Music not found: %s (%s)\n", path, Mix_GetError());
        return;
    }
    gAudio.music[id] = m;
#else
    (void)id; (void)path;
#endif
}

// ── Playback ──────────────────────────────────────────────────────────────────

void playSFX(const std::string& id) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    auto it = gAudio.sfx.find(id);
    if (it == gAudio.sfx.end()) return;
    int vol = (int)(gAudio.masterVolume * MIX_MAX_VOLUME);
    Mix_VolumeChunk(it->second, vol);
    Mix_PlayChannel(-1, it->second, 0);
#else
    (void)id;
#endif
}

void playMusic(const std::string& id, bool loop) {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    if (gAudio.currentMusic == id) return;  // already playing this track
    auto it = gAudio.music.find(id);
    if (it == gAudio.music.end()) return;
    Mix_HaltMusic();
    Mix_VolumeMusic((int)(gAudio.masterVolume * MIX_MAX_VOLUME));
    Mix_PlayMusic(it->second, loop ? -1 : 0);
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
    gAudio.masterVolume = v < 0.f ? 0.f : (v > 1.f ? 1.f : v);
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    int vol = (int)(gAudio.masterVolume * MIX_MAX_VOLUME);
    Mix_Volume(-1, vol);
    Mix_VolumeMusic(vol);
#endif
}

// ── Cleanup ───────────────────────────────────────────────────────────────────

void cleanupAudio() {
#ifdef USE_SDL_MIXER
    if (!gAudio.initialized) return;
    for (auto& [k, c] : gAudio.sfx)   { if (c) Mix_FreeChunk(c); }
    for (auto& [k, m] : gAudio.music) { if (m) Mix_FreeMusic(m); }
    gAudio.sfx.clear();
    gAudio.music.clear();
    Mix_CloseAudio();
    gAudio.initialized = false;
    printf("[Audio] Cleaned up\n");
#endif
}
