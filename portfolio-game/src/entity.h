#pragma once
/**
 * entity.h — Entity base struct and derived types
 * Zielony Pływak | Phase 0.3.3
 */

#include <SDL2/SDL.h>
#include <string>
#include "spritesheet.h"
#include "animation.h"

// ─── Entity (base) ────────────────────────────────────────────────────────────

struct Entity {
    float x = 0, y = 0;          // world-space position (centre)
    float velX = 0, velY = 0;    // velocity px/s
    int   width  = 16;
    int   height = 32;
    bool  active = true;

    SpriteSheet* sheet = nullptr; // nullable — falls back to coloured rect
    AnimationController anim;
};

// ─── Player entity ────────────────────────────────────────────────────────────

struct PlayerEntity : Entity {
    bool  onGround   = false;
    bool  inWater    = false;
    float stamina    = 1.0f;      // 0.0–1.0
    int   carryCount = 0;
    bool  hasBurden  = false;     // carryCount > 10
    bool  facingRight = true;
    float pickupCooldown = 0;
    float animTime = 0;
    int   trashCollected = 0;
    int   totalTrash     = 0;
};

// ─── NPC entity ───────────────────────────────────────────────────────────────

struct NPCEntity : Entity {
    std::string npcId;         // "marta", "tomek", "sela"
    bool        regardGiven    = false;
    bool        favourComplete = false;
};

// ─── Animal entity ────────────────────────────────────────────────────────────

struct AnimalEntity : Entity {
    std::string speciesName;
    float       wanderTimer  = 0;
    float       wanderDirX   = 0;
    bool        spawned      = false;
    float       requiredBI   = 0;  // 0.0–1.0 fraction
    float       animPhase    = 0;
    bool        photographed = false;
};

// ─── Render helper (primitive fallback) ───────────────────────────────────────

inline void renderEntityPrimitive(SDL_Renderer* r, const Entity& e,
                                   int camX, int camY,
                                   SDL_Color col)
{
    int sx = (int)(e.x - e.width  / 2.0f) - camX;
    int sy = (int)(e.y - e.height / 2.0f) - camY;
    SDL_SetRenderDrawBlendMode(r, col.a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
    SDL_Rect rect { sx, sy, e.width, e.height };
    SDL_RenderFillRect(r, &rect);
}
