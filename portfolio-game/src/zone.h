#pragma once
/**
 * zone.h — TileMap data structure and renderer
 * Zielony Pływak | Phase 0.1.1
 *
 * Tile IDs:
 *   0 = air          (not solid, not water)
 *   1 = solid ground (solid, not water)
 *   2 = water surface(not solid, water)
 *   3 = deep water   (not solid, water)
 *   4 = rock         (solid, not water)
 *   5 = platform     (solid, not water — one-way later)
 */

#include <SDL2/SDL.h>
#include <vector>
#include <string>
#include <sstream>

// ─── Tile ────────────────────────────────────────────────────────────────────

struct Tile {
    int  tileId = 0;
    bool solid  = false;
    bool water  = false;
};

// ─── TileMap ─────────────────────────────────────────────────────────────────

struct TileMap {
    std::vector<std::vector<Tile>> tiles; // [row][col]
    int widthInTiles  = 0;
    int heightInTiles = 0;
    int tileSize      = 16; // pixels per tile
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static inline Tile makeTile(int id) {
    Tile t;
    t.tileId = id;
    switch (id) {
        case 1: t.solid = true;  t.water = false; break; // ground
        case 2: t.solid = false; t.water = true;  break; // water surface
        case 3: t.solid = false; t.water = true;  break; // deep water
        case 4: t.solid = true;  t.water = false; break; // rock
        case 5: t.solid = true;  t.water = false; break; // platform
        default: t.solid = false; t.water = false; break; // air
    }
    return t;
}

// ─── loadTileMap ─────────────────────────────────────────────────────────────
// Accepts a multiline string of space-separated integers, one row per line.

static inline TileMap loadTileMap(const std::string& data, int tileSize = 16) {
    TileMap map;
    map.tileSize = tileSize;

    std::istringstream stream(data);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        std::vector<Tile> row;
        std::istringstream lineStream(line);
        int id;
        while (lineStream >> id) {
            row.push_back(makeTile(id));
        }
        if (!row.empty()) {
            map.tiles.push_back(row);
        }
    }

    map.heightInTiles = (int)map.tiles.size();
    map.widthInTiles  = map.heightInTiles > 0 ? (int)map.tiles[0].size() : 0;
    return map;
}

// ─── isSolidTile ─────────────────────────────────────────────────────────────
// Check if a world-space pixel coordinate lands on a solid tile.

static inline bool isSolidTile(const TileMap& map, int pixelX, int pixelY) {
    int col = pixelX / map.tileSize;
    int row = pixelY / map.tileSize;
    if (row < 0 || row >= map.heightInTiles) return false;
    if (col < 0 || col >= map.widthInTiles)  return false;
    return map.tiles[row][col].solid;
}

// ─── isWaterTile ─────────────────────────────────────────────────────────────

static inline bool isWaterTile(const TileMap& map, int pixelX, int pixelY) {
    int col = pixelX / map.tileSize;
    int row = pixelY / map.tileSize;
    if (row < 0 || row >= map.heightInTiles) return false;
    if (col < 0 || col >= map.widthInTiles)  return false;
    return map.tiles[row][col].water;
}

// ─── Placeholder colours per tile type ───────────────────────────────────────

static inline SDL_Color tileColour(int tileId) {
    switch (tileId) {
        case 1:  return {194, 178, 128, 255}; // sand/ground — warm beige
        case 2:  return { 64, 164, 223, 200}; // water surface — bright blue
        case 3:  return { 30,  90, 160, 220}; // deep water — dark blue
        case 4:  return {120, 110,  95, 255}; // rock — grey-brown
        case 5:  return {160, 130,  80, 255}; // platform — darker sand
        default: return {  0,   0,   0,   0}; // air — transparent (skip draw)
    }
}

// ─── drawTileMap ─────────────────────────────────────────────────────────────
// Draws only tiles visible within the 1280×720 viewport.
// cameraX/Y are the world-space pixel coords of the top-left corner.

static inline void drawTileMap(SDL_Renderer* renderer, const TileMap& map,
                                int cameraX, int cameraY,
                                int viewportW = 1280, int viewportH = 720)
{
    const int ts = map.tileSize;

    // Tile range visible on screen
    int colStart = cameraX / ts;
    int rowStart = cameraY / ts;
    int colEnd   = (cameraX + viewportW) / ts + 1;
    int rowEnd   = (cameraY + viewportH) / ts + 1;

    // Clamp to map bounds
    colStart = std::max(0, colStart);
    rowStart = std::max(0, rowStart);
    colEnd   = std::min(map.widthInTiles,  colEnd);
    rowEnd   = std::min(map.heightInTiles, rowEnd);

    for (int row = rowStart; row < rowEnd; ++row) {
        for (int col = colStart; col < colEnd; ++col) {
            const Tile& tile = map.tiles[row][col];
            if (tile.tileId == 0) continue; // skip air

            SDL_Color c = tileColour(tile.tileId);
            SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
            SDL_SetRenderDrawBlendMode(renderer,
                tile.water ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);

            SDL_Rect rect {
                col * ts - cameraX,
                row * ts - cameraY,
                ts,
                ts
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}
