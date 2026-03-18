#pragma once
// ─── Animation System ────────────────────────────────────────────────────────
// Clip-based frame animation controller.
// Works alongside spritesheet.h — use getCurrentFrameIndex() as the frame
// argument passed to drawFrame().

#include <map>
#include <string>

// ── One animation clip ────────────────────────────────────────────────────────
struct AnimationClip {
    int   startFrame    = 0;
    int   frameCount    = 1;
    float frameDuration = 0.1f;   // seconds per frame
    bool  loop          = true;
};

// ── Per-entity controller ─────────────────────────────────────────────────────
struct AnimationController {
    std::map<std::string, AnimationClip> clips;
    std::string currentClip  = "";
    int         currentFrame = 0;
    float       timer        = 0.0f;
    bool        finished     = false;   // true after non-looping clip ends
};

// ── Add a named clip ──────────────────────────────────────────────────────────
inline void addClip(AnimationController& ac, const std::string& name, AnimationClip clip) {
    ac.clips[name] = clip;
}

// ── Switch to a clip (no-op if already playing it) ───────────────────────────
inline void playClip(AnimationController& ac, const std::string& name) {
    if (ac.currentClip == name) return;
    if (ac.clips.find(name) == ac.clips.end()) return;
    ac.currentClip  = name;
    ac.currentFrame = 0;
    ac.timer        = 0.0f;
    ac.finished     = false;
}

// ── Advance animation by deltaTime seconds ───────────────────────────────────
inline void updateAnimation(AnimationController& ac, float deltaTime) {
    if (ac.currentClip.empty()) return;
    auto it = ac.clips.find(ac.currentClip);
    if (it == ac.clips.end()) return;
    const AnimationClip& clip = it->second;
    if (ac.finished) return;

    ac.timer += deltaTime;
    while (ac.timer >= clip.frameDuration) {
        ac.timer -= clip.frameDuration;
        ac.currentFrame++;
        if (ac.currentFrame >= clip.frameCount) {
            if (clip.loop) {
                ac.currentFrame = 0;
            } else {
                ac.currentFrame = clip.frameCount - 1;
                ac.finished     = true;
                break;
            }
        }
    }
}

// ── Absolute frame index into the sprite sheet ───────────────────────────────
inline int getCurrentFrameIndex(const AnimationController& ac) {
    auto it = ac.clips.find(ac.currentClip);
    if (it == ac.clips.end()) return 0;
    return it->second.startFrame + ac.currentFrame;
}

// ── Player clip factory ───────────────────────────────────────────────────────
// Call once after creating a player's AnimationController.
inline void registerPlayerClips(AnimationController& ac) {
    addClip(ac, "idle",   { 0,  4, 0.20f, true  });
    addClip(ac, "walk",   { 4,  6, 0.10f, true  });
    addClip(ac, "jump",   { 10, 3, 0.15f, false });
    addClip(ac, "pickup", { 13, 3, 0.12f, false });
    playClip(ac, "idle");
}
