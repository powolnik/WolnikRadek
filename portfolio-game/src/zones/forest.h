#pragma once
/**
 * zones/forest.h — Forest Zone
 * Unlock at BI 60% (50% if sela_helped)
 * Birch grove, varied elevation, 3 floating platforms
 */
#include "../zone.h"

static inline TileMap buildForestZone() {
    const int W = 240, H = 45, TS = 16;
    TileMap map;
    map.tileSize = TS;
    map.widthInTiles  = W;
    map.heightInTiles = H;
    map.tiles.assign(H, std::vector<Tile>(W, makeTile(0)));

    // Varied ground height: oscillates between row 29 and row 36
    // Use a simple pattern: every ~16 cols, raise or lower by 1-2 rows
    for (int c = 0; c < W; ++c) {
        // Ground row oscillates: base=33, +/-4 via sine approximation
        int phase = (c / 30) % 4;
        int groundRow;
        switch (phase) {
            case 0: groundRow = 33; break;
            case 1: groundRow = 31; break;
            case 2: groundRow = 35; break;
            default: groundRow = 32; break;
        }
        // Extra variation at local level
        if ((c % 7) == 0) groundRow = std::max(29, groundRow - 2);
        if ((c % 11) == 0) groundRow = std::min(36, groundRow + 2);

        for (int r = groundRow; r < H; ++r)
            map.tiles[r][c] = makeTile(1);
    }

    // Rock clusters (overwrite a few ground surface tiles)
    struct RockGroup { int c, cEnd, r; };
    RockGroup rocks[] = {
        {10, 14, 0},   // group 1 — 0 means "on top of ground"
        {40, 43, 0},
        {70, 74, 0},
        {100, 103, 0},
        {130, 133, 0},
        {160, 165, 0},
        {190, 194, 0},
        {220, 224, 0}
    };
    for (auto& rg : rocks) {
        for (int c = rg.c; c <= rg.cEnd; ++c) {
            // find ground row for this col
            for (int r = 0; r < H; ++r) {
                if (map.tiles[r][c].solid) {
                    map.tiles[r][c] = makeTile(4);
                    if (r > 0) map.tiles[r-1][c] = makeTile(4);
                    break;
                }
            }
        }
    }

    // 3 floating platforms (5 tiles wide, 6-8 tiles above ground)
    // Find ground at target cols and place platform above
    auto findGround = [&](int col) -> int {
        for (int r = 0; r < H; ++r)
            if (map.tiles[r][col].solid) return r;
        return H - 8;
    };

    // Platform 1: centred at col 50
    {
        int gr = findGround(50);
        int pr = std::max(0, gr - 8);
        for (int c = 48; c <= 52; ++c)
            map.tiles[pr][c] = makeTile(5);
    }
    // Platform 2: centred at col 120
    {
        int gr = findGround(120);
        int pr = std::max(0, gr - 7);
        for (int c = 118; c <= 122; ++c)
            map.tiles[pr][c] = makeTile(5);
    }
    // Platform 3: centred at col 190
    {
        int gr = findGround(190);
        int pr = std::max(0, gr - 9);
        for (int c = 188; c <= 192; ++c)
            map.tiles[pr][c] = makeTile(5);
    }

    return map;
}
