#include "spritesheet.h"

// ─── loadSpriteSheet ─────────────────────────────────────────────────────────

SpriteSheet loadSpriteSheet(SDL_Renderer* renderer,
                             const char*   path,
                             int           frameW,
                             int           frameH)
{
    SpriteSheet sheet;
    sheet.frameW = frameW;
    sheet.frameH = frameH;

    SDL_Texture* tex = IMG_LoadTexture(renderer, path);
    if (!tex) {
        fprintf(stderr, "[SpriteSheet] WARNING: could not load '%s': %s\n",
                path, IMG_GetError());
        return sheet;   // texture stays nullptr — callers must handle this
    }

    // Query actual texture dimensions to derive frame layout
    int texW = 0, texH = 0;
    SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

    sheet.texture     = tex;
    sheet.columns     = (frameW > 0) ? (texW / frameW) : 1;
    int rows          = (frameH > 0) ? (texH / frameH) : 1;
    sheet.totalFrames = sheet.columns * rows;

    return sheet;
}

// ─── drawFrame ───────────────────────────────────────────────────────────────

void drawFrame(SDL_Renderer*      renderer,
               const SpriteSheet& sheet,
               int                frameIndex,
               int                destX,
               int                destY,
               bool               flipH,
               int                scale)
{
    if (!sheet.texture) return;

    // Clamp frameIndex to valid range
    int fi = frameIndex % sheet.totalFrames;
    if (fi < 0) fi = 0;

    int col = fi % sheet.columns;
    int row = fi / sheet.columns;

    SDL_Rect src = {
        col * sheet.frameW,
        row * sheet.frameH,
        sheet.frameW,
        sheet.frameH
    };

    SDL_Rect dst = {
        destX,
        destY,
        sheet.frameW * scale,
        sheet.frameH * scale
    };

    SDL_RendererFlip flip = flipH ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

    SDL_RenderCopyEx(renderer, sheet.texture,
                     &src, &dst,
                     0.0,          // angle
                     nullptr,      // rotate around centre
                     flip);
}

// ─── destroySpriteSheet ──────────────────────────────────────────────────────

void destroySpriteSheet(SpriteSheet& sheet)
{
    if (sheet.texture) {
        SDL_DestroyTexture(sheet.texture);
        sheet.texture = nullptr;
    }
}
