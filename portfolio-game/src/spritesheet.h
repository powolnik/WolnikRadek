#pragma once
/**
 * spritesheet.h / spritesheet.cpp
 * --------------------------------
 * Sprite sheet loader and frame renderer for the Zielony Pływak game.
 * Loads PNG sprite sheets using SDL2_image and draws individual frames
 * with optional horizontal flip and 2× scale.
 *
 * Requires:  -s USE_SDL_IMAGE=2  -s SDL2_IMAGE_FORMATS='["png"]'  in Emscripten flags.
 * Falls back gracefully (null texture, warning to stderr) if a PNG is missing.
 */

#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>

// ─── Data ────────────────────────────────────────────────────────────────────

struct SpriteSheet {
    SDL_Texture* texture    = nullptr;
    int          frameW     = 16;   // source width  of one frame (pixels)
    int          frameH     = 16;   // source height of one frame (pixels)
    int          columns    = 1;    // number of frames per row
    int          totalFrames = 1;   // total frames in the sheet
};

// ─── API ─────────────────────────────────────────────────────────────────────

/**
 * Load a sprite sheet PNG from `path`.
 * frameW / frameH define the size of a single animation frame in the source image.
 * Returns a SpriteSheet with texture == nullptr if the file cannot be loaded.
 */
SpriteSheet loadSpriteSheet(SDL_Renderer* renderer,
                             const char*   path,
                             int           frameW,
                             int           frameH);

/**
 * Draw one frame from the sheet at (destX, destY) in screen space.
 * Frames are numbered left-to-right, top-to-bottom, starting at 0.
 * scale — default 2 renders 16-px art as 32-px on screen.
 * flipH — mirrors the sprite horizontally (for left-facing movement).
 * Returns early and does nothing if sheet.texture == nullptr.
 */
void drawFrame(SDL_Renderer*       renderer,
               const SpriteSheet&  sheet,
               int                 frameIndex,
               int                 destX,
               int                 destY,
               bool                flipH  = false,
               int                 scale  = 2);

/**
 * Free the GPU texture held by the sheet.
 * Safe to call on a sheet with texture == nullptr.
 */
void destroySpriteSheet(SpriteSheet& sheet);
