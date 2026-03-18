#pragma once
/**
 * zones/meadow.h — Meadow Zone
 * Unlock at BI 40%
 * Flat coastal grass, scattered rocks, 2 shallow puddles
 */
#include "../zone.h"

static inline TileMap buildMeadowZone() {
    const int W = 240, H = 45, TS = 16;
    TileMap map;
    map.tileSize = TS;
    map.widthInTiles  = W;
    map.heightInTiles = H;
    map.tiles.assign(H, std::vector<Tile>(W, makeTile(0)));

    // Bottom 8 rows: flat ground (rows 37-44)
    for (int r = 37; r < H; ++r)
        for (int c = 0; c < W; ++c)
            map.tiles[r][c] = makeTile(1);

    // Scattered rocks on surface (row 36)
    int rockGroups[] = {
        20, 21, 22,      // cluster 1
        55, 56,          // cluster 2
        90, 91, 92, 93,  // cluster 3
        130, 131,        // cluster 4
        170, 171, 172,   // cluster 5
        210, 211         // cluster 6
    };
    for (int rc : rockGroups) {
        map.tiles[36][rc] = makeTile(4);
        map.tiles[37][rc] = makeTile(4); // rocks are 2 tiles tall
    }

    // Puddle 1: cols 60-75, rows 37-38 as water (depression)
    for (int c = 60; c <= 75; ++c) {
        map.tiles[37][c] = makeTile(2);   // water surface
        map.tiles[38][c] = makeTile(3);   // deep water
        map.tiles[39][c] = makeTile(1);   // solid bottom
        map.tiles[40][c] = makeTile(1);
        map.tiles[41][c] = makeTile(1);
        map.tiles[42][c] = makeTile(1);
        map.tiles[43][c] = makeTile(1);
        map.tiles[44][c] = makeTile(1);
    }

    // Puddle 2: cols 150-165
    for (int c = 150; c <= 165; ++c) {
        map.tiles[37][c] = makeTile(2);
        map.tiles[38][c] = makeTile(3);
        map.tiles[39][c] = makeTile(1);
        map.tiles[40][c] = makeTile(1);
        map.tiles[41][c] = makeTile(1);
        map.tiles[42][c] = makeTile(1);
        map.tiles[43][c] = makeTile(1);
        map.tiles[44][c] = makeTile(1);
    }

    return map;
}
