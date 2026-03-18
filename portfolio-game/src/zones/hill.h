#pragma once
/**
 * zones/hill.h — Hill Zone
 * Unlock at BI 80%
 * Rocky summit, ascending terrain, narrow ledge paths, broadcast tower
 */
#include "../zone.h"
#include <algorithm>

static inline TileMap buildHillZone() {
    const int W = 240, H = 45, TS = 16;
    TileMap map;
    map.tileSize = TS;
    map.widthInTiles  = W;
    map.heightInTiles = H;
    map.tiles.assign(H, std::vector<Tile>(W, makeTile(0)));

    // Ascending terrain: ground rises from row 36 at col 0 to row 18 at col 239
    for (int c = 0; c < W; ++c) {
        float t = (float)c / (W - 1);
        int groundRow = (int)(36 - t * 18);  // 36 at left, 18 at right
        groundRow = std::max(18, std::min(38, groundRow));

        for (int r = groundRow; r < H; ++r)
            map.tiles[r][c] = makeTile(1);
    }

    // Dense rock clusters (overwrite surface tiles)
    int rockCols[] = {
        5, 6, 12, 13, 14,
        25, 26, 27,
        38, 39, 40, 41,
        55, 56, 60, 61,
        75, 76, 77, 78,
        90, 91, 95, 96,
        110, 111, 115, 116, 117,
        130, 131, 135, 136,
        150, 151, 152, 155, 156,
        170, 171, 175, 176, 177,
        190, 191, 195, 196,
        210, 211, 215, 216, 220, 221
    };
    for (int rc : rockCols) {
        if (rc >= W) continue;
        for (int r = 0; r < H; ++r) {
            if (map.tiles[r][rc].solid) {
                map.tiles[r][rc] = makeTile(4);
                if (r > 0) map.tiles[r-1][rc] = makeTile(4);
                break;
            }
        }
    }

    // 3 narrow ledge platforms (8 tiles wide, staggered for jump challenge)
    auto findGround = [&](int col) -> int {
        for (int r = 0; r < H; ++r)
            if (map.tiles[r][col].solid) return r;
        return H - 6;
    };

    // Ledge 1: cols 30-37
    {
        int gr = findGround(33);
        int pr = std::max(0, gr - 7);
        for (int c = 30; c <= 37; ++c)
            map.tiles[pr][c] = makeTile(5);
    }
    // Ledge 2: cols 90-97
    {
        int gr = findGround(93);
        int pr = std::max(0, gr - 9);
        for (int c = 90; c <= 97; ++c)
            map.tiles[pr][c] = makeTile(5);
    }
    // Ledge 3: cols 160-167
    {
        int gr = findGround(163);
        int pr = std::max(0, gr - 11);
        for (int c = 160; c <= 167; ++c)
            map.tiles[pr][c] = makeTile(5);
    }

    return map;
}
