#pragma once
/**
 * zones/shallows.h — Shallows Zone
 * Unlock at BI 20%
 * Tidal pools, seafloor, water surface, 3 sand islands
 */
#include "../zone.h"

static inline TileMap buildShallowsZone() {
    const int W = 240, H = 45, TS = 16;
    TileMap map;
    map.tileSize = TS;
    map.widthInTiles  = W;
    map.heightInTiles = H;
    map.tiles.assign(H, std::vector<Tile>(W, makeTile(0)));

    // Bottom 6 rows: solid seafloor (rows 39-44)
    for (int r = 39; r < H; ++r)
        for (int c = 0; c < W; ++c)
            map.tiles[r][c] = makeTile(1);

    // Rows 33-38: deep water
    for (int r = 33; r < 39; ++r)
        for (int c = 0; c < W; ++c)
            map.tiles[r][c] = makeTile(3);

    // Row 32: water surface
    for (int c = 0; c < W; ++c)
        map.tiles[32][c] = makeTile(2);

    // Scattered rocks on seafloor (row 38, every ~20 cols in groups)
    int rockCols[] = {15, 16, 17, 45, 46, 80, 81, 120, 121, 122, 165, 166, 200, 201, 202, 220, 221};
    for (int rc : rockCols)
        map.tiles[38][rc] = makeTile(4);

    // 3 sand islands (platforms sticking out of water)
    // Island 1: cols 30-42, row 31 (surface at row 31, solid below)
    for (int c = 30; c <= 42; ++c) {
        map.tiles[31][c] = makeTile(1);  // surface (above water surface)
        map.tiles[32][c] = makeTile(1);  // fills down
        map.tiles[33][c] = makeTile(1);
        map.tiles[34][c] = makeTile(1);
    }

    // Island 2: cols 100-112, row 30
    for (int c = 100; c <= 112; ++c) {
        map.tiles[30][c] = makeTile(1);
        map.tiles[31][c] = makeTile(1);
        map.tiles[32][c] = makeTile(1);
        map.tiles[33][c] = makeTile(1);
        map.tiles[34][c] = makeTile(1);
    }

    // Island 3: cols 180-195, row 28
    for (int c = 180; c <= 195; ++c) {
        map.tiles[28][c] = makeTile(1);
        map.tiles[29][c] = makeTile(1);
        map.tiles[30][c] = makeTile(1);
        map.tiles[31][c] = makeTile(1);
        map.tiles[32][c] = makeTile(1);
        map.tiles[33][c] = makeTile(1);
        map.tiles[34][c] = makeTile(1);
    }

    return map;
}
