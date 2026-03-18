/**
 * Zielony Pływak (The Green Floater)
 * C++ / SDL2 / Emscripten → WebAssembly
 *
 * A cosy 2D side-scrolling cleanup game set on a polluted island.
 * Player (Kai) explores 3 zones, picks up trash, restores wildlife.
 *
 * Build: emcc src/main.cpp -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1
 *        -O2 -o game.js --shell-file shell.html
 */

#include <SDL2/SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>
#include <string>
#include <algorithm>

// CHANGED: TileMap system replacing procedural terrain
#include "zone.h"
#include "zones/beach.h"
#include "zones/shallows.h"
#include "zones/meadow.h"
#include "zones/forest.h"
#include "zones/hill.h"
#include "spritesheet.h"
#include "animation.h"
#include "entity.h"
#include "audio.h"

// ─── Constants ───────────────────────────────────────────────────────────────

constexpr int   WINDOW_W      = 1280;
constexpr int   WINDOW_H      = 720;
constexpr float GRAVITY       = 980.0f;
constexpr float PLAYER_SPEED  = 220.0f;
constexpr float JUMP_FORCE    = 420.0f;
constexpr float SWIM_SPEED    = 160.0f;
constexpr float PICKUP_RANGE  = 48.0f;
// CHANGED: world now matches Beach tilemap dimensions (240×45 tiles × 16px)
constexpr int   WORLD_W       = 3840; // 240 tiles × 16px
constexpr int   WORLD_H       = 720;  // 45 tiles × 16px (1 viewport tall)

static constexpr int   NUM_ZONES = 5;
static constexpr float ZONE_BI_THRESHOLDS[NUM_ZONES] = { 0.0f, 0.20f, 0.40f, 0.60f, 0.80f };
static const char*     ZONE_DISPLAY_NAMES[NUM_ZONES]  = {
    "~ THE BEACH ~", "~ THE SHALLOWS ~", "~ THE MEADOW ~",
    "~ THE FOREST ~", "~ THE HILL ~"
};
// Colours used for zone gate line (locked vs unlocked)
static const Color COL_GATE_LOCKED   = {180,  60,  60, 220};
static const Color COL_GATE_UNLOCKED = { 60, 180,  80, 220};

// Zone boundaries (world X coords)
constexpr int ZONE_BEACH_START   = 0;
constexpr int ZONE_BEACH_END     = 3840;   // 240 tiles × 16px (current tilemap)
constexpr int ZONE_FOREST_START  = 3840;   // placeholder — tilemap not yet loaded
constexpr int ZONE_FOREST_END    = 7680;
constexpr int ZONE_WETLAND_START = 7680;   // placeholder — tilemap not yet loaded
constexpr int ZONE_WETLAND_END   = 11520;

// Colors (solarpunk palette)
struct Color { Uint8 r, g, b, a; };

static const Color COL_SKY_TOP     = {135, 206, 235, 255};
static const Color COL_SKY_BOT     = {200, 230, 250, 255};
static const Color COL_WATER       = { 40, 120, 180, 180};
static const Color COL_WATER_DEEP  = { 20,  60, 120, 200};
static const Color COL_SAND        = {220, 200, 160, 255};
static const Color COL_GRASS       = { 60, 160,  60, 255};
static const Color COL_DARK_GRASS  = { 40, 120,  40, 255};
static const Color COL_MUD         = {100,  80,  50, 255};
static const Color COL_TREE_TRUNK  = {100,  70,  40, 255};
static const Color COL_TREE_LEAVES = { 30, 140,  50, 255};
static const Color COL_PLAYER_BODY = { 50, 180,  80, 255};
static const Color COL_PLAYER_HEAD = {230, 190, 150, 255};
static const Color COL_TRASH       = {180,  60,  60, 255};
static const Color COL_TRASH_GLOW  = {255, 100, 100, 120};
static const Color COL_ANIMAL      = {200, 160,  80, 255};
static const Color COL_BIRD        = { 80, 140, 200, 255};
static const Color COL_FISH        = {100, 200, 180, 255};
static const Color COL_UI_BG       = { 10,  10,  26, 200};
static const Color COL_UI_ACCENT   = { 96,  96, 255, 255};
static const Color COL_ECO_LOW     = {200,  60,  60, 255};
static const Color COL_ECO_MID     = {220, 180,  40, 255};
static const Color COL_ECO_HIGH    = { 60, 200,  80, 255};
static const Color COL_WHITE       = {255, 255, 255, 255};
static const Color COL_BLACK       = {  0,   0,   0, 255};

// ─── Enums ───────────────────────────────────────────────────────────────────

enum class GameScene { MENU, PLAYING, PAUSED, WIN };
enum class Zone { BEACH, FOREST, WETLAND };
enum class TrashType { BOTTLE, CAN, BAG, TIRE, BARREL };
enum class AnimalType { CRAB, BIRD, DEER, FROG, FISH };

// ─── Structs ─────────────────────────────────────────────────────────────────

struct Vec2 {
    float x = 0, y = 0;
    Vec2 operator+(Vec2 o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(Vec2 o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
    float len() const { return std::sqrt(x*x + y*y); }
};

struct Player {
    Vec2 pos = {200, 500};
    Vec2 vel = {0, 0};
    bool onGround  = false;
    bool inWater   = false;
    bool facingRight = true;
    int  trashCollected = 0;
    int  totalTrash = 0;
    float animTime = 0;
    float pickupCooldown = 0;
    AnimationController anim;   // clip-based animation controller
    // Simple bounding box
    float w = 24, h = 48;
};

struct TrashItem {
    Vec2 pos;
    TrashType type;
    bool collected = false;
    float bobPhase = 0;   // for floating trash
    bool inWater = false;
};

struct Animal {
    Vec2 pos;
    AnimalType type;
    bool spawned = false;       // appears after enough cleanup
    float requiredEco = 0;      // eco% needed to appear
    float animPhase = 0;
    Vec2 moveDir = {0, 0};
    float moveTimer = 0;
};

struct Cloud {
    float x, y, w, speed;
};

struct Particle {
    Vec2 pos, vel;
    float life, maxLife;
    Color col;
};

struct TerrainSegment {
    int x1, x2;
    int groundY;       // Y coordinate of ground surface at this segment
    bool isWater;
};

struct Decoration {
    Vec2 pos;
    int type;  // 0=bush, 1=flower, 2=rock, 3=reed, 4=lily
    float scale;
};

// ─── Game State ──────────────────────────────────────────────────────────────

struct GameState {
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Timing
    Uint32  lastTick  = 0;
    float   deltaTime = 0.0f;
    float   totalTime = 0.0f;
    Uint64  frameCount = 0;

    // Input
    struct Input {
        bool left = false, right = false, up = false, down = false;
        bool jump = false;
        bool action = false;   // E key / pickup
        bool pause = false;
        bool enter = false;
        bool jumpPressed = false;  // single frame
        bool actionPressed = false;
        bool pausePressed = false;
        bool enterPressed = false;
    } input;

    // Scene
    GameScene scene = GameScene::MENU;

    // Camera
    Vec2 camera = {0, 0};

    // World
    Player player;
    std::vector<TrashItem>       trash;
    std::vector<Animal>          animals;
    std::vector<Cloud>           clouds;
    std::vector<Particle>        particles;
    std::vector<TerrainSegment>  terrain;
    std::vector<Decoration>      decorations;

    // CHANGED: TileMap-based terrain (replaces procedural heightmap)
    TileMap currentZoneMap;

    // Zone management (Phase 1)
    TileMap  allZones[NUM_ZONES];       // preloaded at startup
    int      currentZoneIdx = 0;
    float    flashMsgTimer  = 0;        // for "Need X% BI" block message
    std::string flashMsg;               // block message text (unused visually yet, stored)
    bool     zoneTransitioning = false; // brief input-disable after zone switch
    float    zoneTransitionTimer = 0;

    // Player sprite sheet (loaded from PNG or placeholder at runtime)
    SpriteSheet playerSheet;

    // Eco meter (0-100)
    float ecoMeter = 0;
    float targetEco = 0;

    // Messages
    std::string popupMsg;
    float popupTimer = 0;

    // Zone name display
    std::string currentZoneName;
    float zoneNameTimer = 0;
    Zone lastZone = Zone::BEACH;

    // Win state
    bool allTrashCollected = false;

    // Menu
    int menuSelection = 0;
    float menuAnim = 0;
};

static GameState g;

// ─── Helpers ─────────────────────────────────────────────────────────────────

static void setColor(Color c) {
    SDL_SetRenderDrawBlendMode(g.renderer, c.a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(g.renderer, c.r, c.g, c.b, c.a);
}

static void drawRect(int x, int y, int w, int h, Color c) {
    setColor(c);
    SDL_Rect r = {x, y, w, h};
    SDL_RenderFillRect(g.renderer, &r);
}

static void drawRectWorld(float wx, float wy, int w, int h, Color c) {
    int sx = (int)(wx - g.camera.x);
    int sy = (int)(wy - g.camera.y);
    drawRect(sx, sy, w, h, c);
}

static void drawCircle(int cx, int cy, int radius, Color c) {
    setColor(c);
    for (int dy = -radius; dy <= radius; dy++) {
        int dx = (int)std::sqrt(radius*radius - dy*dy);
        SDL_RenderDrawLine(g.renderer, cx-dx, cy+dy, cx+dx, cy+dy);
    }
}

static void drawCircleWorld(float wx, float wy, int radius, Color c) {
    int sx = (int)(wx - g.camera.x);
    int sy = (int)(wy - g.camera.y);
    drawCircle(sx, sy, radius, c);
}

static float lerp(float a, float b, float t) { return a + (b-a)*t; }

static Color lerpColor(Color a, Color b, float t) {
    return {
        (Uint8)(a.r + (b.r - a.r)*t),
        (Uint8)(a.g + (b.g - a.g)*t),
        (Uint8)(a.b + (b.b - a.b)*t),
        (Uint8)(a.a + (b.a - a.a)*t)
    };
}

static float randf() { return (float)rand() / (float)RAND_MAX; }
static float randf(float lo, float hi) { return lo + randf()*(hi-lo); }

static int getGroundY(float worldX) {
    // Terrain profile: beach slopes down to water, forest is hilly, wetland is flat with ponds
    float x = worldX;

    if (x < 0) return 700;
    if (x >= WORLD_W) return 700;

    // Beach zone: gentle slope to water edge
    if (x < ZONE_BEACH_END) {
        float t = x / (float)ZONE_BEACH_END;
        // Start high, dip down near water at edges
        float base = 620.0f;
        // Small dunes
        base += 15.0f * std::sin(x * 0.008f);
        base += 8.0f * std::sin(x * 0.02f);
        // Shore water area
        if (x < 300) {
            base = lerp(750.0f, base, x / 300.0f);
        }
        return (int)base;
    }

    // Forest zone: hilly
    if (x < ZONE_FOREST_END) {
        float lx = x - ZONE_FOREST_START;
        float base = 580.0f;
        base += 40.0f * std::sin(lx * 0.004f);
        base += 20.0f * std::sin(lx * 0.012f);
        base += 10.0f * std::sin(lx * 0.025f);
        return (int)base;
    }

    // Wetland zone: flat with water pools
    {
        float lx = x - ZONE_WETLAND_START;
        float base = 640.0f;
        base += 10.0f * std::sin(lx * 0.006f);
        base += 5.0f * std::sin(lx * 0.018f);
        return (int)base;
    }
}

static bool isWaterAt(float worldX, float worldY) {
    // Beach: water on left edge
    if (worldX < 250 && worldY > 680) return true;

    // Wetland: pools
    if (worldX > ZONE_WETLAND_START) {
        float lx = worldX - ZONE_WETLAND_START;
        // Several pool locations
        float pool1Center = 400, pool2Center = 1000, pool3Center = 1600;
        float poolWidth = 200;
        int gy = getGroundY(worldX);
        if (worldY > gy - 5) {
            if (std::abs(lx - pool1Center) < poolWidth ||
                std::abs(lx - pool2Center) < poolWidth ||
                std::abs(lx - pool3Center) < poolWidth) {
                return true;
            }
        }
    }
    return false;
}

static int getWaterLevel(float worldX) {
    if (worldX < 250) return 700;
    if (worldX > ZONE_WETLAND_START) {
        return getGroundY(worldX) + 20;
    }
    return 9999; // no water
}

static Zone getZone(float worldX) {
    if (worldX < ZONE_BEACH_END) return Zone::BEACH;
    if (worldX < ZONE_FOREST_END) return Zone::FOREST;
    return Zone::WETLAND;
}

static const char* getZoneName(Zone z) {
    switch(z) {
        case Zone::BEACH:   return "~ THE BEACH ~";
        case Zone::FOREST:  return "~ THE FOREST ~";
        case Zone::WETLAND: return "~ THE WETLANDS ~";
    }
    return "";
}

// ─── Particle System ─────────────────────────────────────────────────────────

static void spawnParticles(Vec2 pos, Color col, int count, float speed = 80.0f) {
    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        float angle = randf(0, 6.28f);
        float spd = randf(speed * 0.3f, speed);
        p.vel = {std::cos(angle)*spd, std::sin(angle)*spd - 40.0f};
        p.maxLife = p.life = randf(0.3f, 0.8f);
        p.col = col;
        g.particles.push_back(p);
    }
}

// ─── World Generation ────────────────────────────────────────────────────────

// ── Placeholder 16-frame player sprite sheet (256×32) ────────────────────────
// 16 frames × 16px wide × 32px tall. Each frame is a coloured rectangle with
// a small white marker so you can see which frame is active.
static SDL_Texture* createPlaceholderPlayerSheet(SDL_Renderer* renderer) {
    const int FW = 16, FH = 32, FRAMES = 16;
    SDL_Texture* tex = SDL_CreateTexture(renderer,
                                         SDL_PIXELFORMAT_RGBA8888,
                                         SDL_TEXTUREACCESS_TARGET,
                                         FW * FRAMES, FH);
    if (!tex) return nullptr;
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetRenderTarget(renderer, tex);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (int i = 0; i < FRAMES; i++) {
        int ox = i * FW;
        // Body: green with slight hue shift per frame for visibility
        Uint8 g_val = (Uint8)(140 + i * 6);
        SDL_SetRenderDrawColor(renderer, 30, g_val, 60, 255);
        SDL_Rect body = {ox + 3, 8, 10, 20};
        SDL_RenderFillRect(renderer, &body);

        // Head
        SDL_SetRenderDrawColor(renderer, 210, 160, 110, 255);
        SDL_Rect head = {ox + 4, 1, 8, 8};
        SDL_RenderFillRect(renderer, &head);

        // Legs
        SDL_SetRenderDrawColor(renderer, 20, 80, 40, 255);
        SDL_Rect legs = {ox + 3, 28, 10, 4};
        SDL_RenderFillRect(renderer, &legs);

        // White dot — frame indicator (top-left corner of frame)
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 180);
        SDL_Rect dot = {ox + 1, 1, 2, 2};
        SDL_RenderFillRect(renderer, &dot);
        // Extra dots for frame index (binary-ish visual)
        if (i & 1) { SDL_Rect d2 = {ox+1, 4, 2, 2}; SDL_RenderFillRect(renderer, &d2); }
        if (i & 2) { SDL_Rect d3 = {ox+4, 1, 2, 2}; SDL_RenderFillRect(renderer, &d3); }
        if (i & 4) { SDL_Rect d4 = {ox+4, 4, 2, 2}; SDL_RenderFillRect(renderer, &d4); }
    }

    SDL_SetRenderTarget(renderer, nullptr);
    return tex;
}

// Returns approximate ground Y pixel for a tilemap column
static int getTilemapGroundY(const TileMap& map, int pixelX) {
    int col = pixelX / map.tileSize;
    if (col < 0) col = 0;
    if (col >= map.widthInTiles) col = map.widthInTiles - 1;
    for (int row = 0; row < map.heightInTiles; ++row) {
        if (map.tiles[row][col].solid) {
            return row * map.tileSize;  // top pixel of first solid tile
        }
    }
    return map.heightInTiles * map.tileSize; // fallback: bottom
}

static void generateZoneTrash(int zoneIdx) {
    g.trash.clear();

    // Number of trash items per zone and their types
    struct TrashSpec { float minX, maxX; TrashType type; bool water; };
    std::vector<TrashSpec> specs;

    const TileMap& zmap = g.allZones[zoneIdx];

    switch (zoneIdx) {
        case 0: // Beach
            specs = {
                {100,  500, TrashType::BOTTLE, false},
                {100,  500, TrashType::BOTTLE, false},
                {100,  500, TrashType::BOTTLE, false},
                {100,  500, TrashType::BOTTLE, false},
                {100,  500, TrashType::BOTTLE, false},
                {500, 1200, TrashType::CAN, false},
                {500, 1200, TrashType::CAN, false},
                {500, 1200, TrashType::CAN, false},
                {500, 1200, TrashType::CAN, false},
                {500, 1200, TrashType::CAN, false},
                {800, 2000, TrashType::BAG, false},
                {800, 2000, TrashType::BAG, false},
                {800, 2000, TrashType::BAG, false},
                {800, 2000, TrashType::BAG, false},
                {800, 2000, TrashType::BAG, false},
            };
            break;
        case 1: // Shallows
            specs = {
                {200, 600, TrashType::BARREL, true},
                {200, 600, TrashType::BARREL, true},
                {200, 600, TrashType::BARREL, true},
                {600, 1200, TrashType::BOTTLE, true},
                {600, 1200, TrashType::BOTTLE, true},
                {600, 1200, TrashType::BOTTLE, true},
                {1200, 2000, TrashType::CAN, false},
                {1200, 2000, TrashType::CAN, false},
                {1200, 2000, TrashType::CAN, false},
                {2000, 3000, TrashType::BAG, false},
                {2000, 3000, TrashType::BAG, false},
                {2000, 3000, TrashType::BAG, false},
            };
            break;
        case 2: // Meadow
            specs = {
                {100, 800, TrashType::BARREL, false},
                {100, 800, TrashType::BARREL, false},
                {100, 800, TrashType::BARREL, false},
                {800, 1500, TrashType::CAN, false},
                {800, 1500, TrashType::CAN, false},
                {800, 1500, TrashType::CAN, false},
                {1500, 2500, TrashType::BAG, false},
                {1500, 2500, TrashType::BAG, false},
                {1500, 2500, TrashType::BAG, false},
                {2500, 3500, TrashType::BOTTLE, false},
                {2500, 3500, TrashType::BOTTLE, false},
                {2500, 3500, TrashType::BOTTLE, false},
            };
            break;
        case 3: // Forest
            specs = {
                {200,  800, TrashType::TIRE, false},
                {200,  800, TrashType::TIRE, false},
                {200,  800, TrashType::TIRE, false},
                {800, 1600, TrashType::BARREL, false},
                {800, 1600, TrashType::BARREL, false},
                {800, 1600, TrashType::BARREL, false},
                {1600, 2500, TrashType::CAN, false},
                {1600, 2500, TrashType::CAN, false},
                {1600, 2500, TrashType::CAN, false},
                {2500, 3500, TrashType::BAG, false},
                {2500, 3500, TrashType::BAG, false},
                {2500, 3500, TrashType::BAG, false},
            };
            break;
        case 4: // Hill
            specs = {
                {300, 900, TrashType::TIRE, false},
                {300, 900, TrashType::TIRE, false},
                {900, 1800, TrashType::BARREL, false},
                {900, 1800, TrashType::BARREL, false},
                {1800, 2800, TrashType::TIRE, false},
                {1800, 2800, TrashType::TIRE, false},
                {2800, 3500, TrashType::CAN, false},
                {2800, 3500, TrashType::CAN, false},
                {2800, 3500, TrashType::BAG, false},
            };
            break;
    }

    for (auto& spec : specs) {
        TrashItem item;
        item.pos.x = randf(spec.minX, spec.maxX);
        int gy = getTilemapGroundY(zmap, (int)item.pos.x);
        item.pos.y = (float)(gy - 8);
        item.type    = spec.type;
        item.inWater = spec.water;
        item.bobPhase = randf(0, 6.28f);
        g.trash.push_back(item);
    }

    g.player.totalTrash     = (int)g.trash.size();
    g.player.trashCollected = 0;
}

static void switchZone(int newIdx) {
    if (newIdx < 0 || newIdx >= NUM_ZONES) return;
    g.currentZoneIdx  = newIdx;
    g.currentZoneMap  = g.allZones[newIdx];
    g.player.pos      = {100.0f, 300.0f};   // will fall to ground
    g.player.vel      = {0, 0};
    g.camera          = {0, 0};
    g.currentZoneName = ZONE_DISPLAY_NAMES[newIdx];
    g.zoneNameTimer   = 3.0f;
    g.zoneTransitioning  = true;
    g.zoneTransitionTimer = 0.6f;  // 0.6s input disable
    generateZoneTrash(newIdx);
    // Reset animals for new zone
    g.animals.clear();
    // Spawn a few zone-specific animals (appear at low eco)
    auto addA = [](float x, float y, AnimalType t, float eco){
        Animal a; a.pos={x,y}; a.type=t; a.requiredEco=eco;
        a.animPhase=randf(0,6.28f); a.moveTimer=randf(1,4);
        g.animals.push_back(a);
    };
    switch(newIdx){
        case 0: addA(350,680,AnimalType::CRAB,10); addA(800,500,AnimalType::BIRD,25); break;
        case 1: addA(600,520,AnimalType::FISH,0);  addA(1500,500,AnimalType::BIRD,15); break;
        case 2: addA(400,640,AnimalType::BIRD,5);  addA(2000,640,AnimalType::BIRD,20); break;
        case 3: addA(600,520,AnimalType::DEER,5);  addA(1800,480,AnimalType::BIRD,20); break;
        case 4: addA(800,480,AnimalType::BIRD,0);  addA(2200,460,AnimalType::BIRD,15); break;
    }
}

static void generateWorld() {
    srand((unsigned)time(nullptr));

    // CHANGED: build Beach tilemap (replaces procedural heightmap)
    g.currentZoneMap = buildBeachZone();

    // Preload all zone tilemaps
    g.allZones[0] = g.currentZoneMap;  // Beach already built
    g.allZones[1] = buildShallowsZone();
    g.allZones[2] = buildMeadowZone();
    g.allZones[3] = buildForestZone();
    g.allZones[4] = buildHillZone();
    g.currentZoneIdx = 0;

    // Init player sprite sheet — try real PNG first, fall back to placeholder
    if (g.playerSheet.texture) { destroySpriteSheet(g.playerSheet); }
    g.playerSheet = loadSpriteSheet(g.renderer, "assets/player.png", 16, 32);
    if (!g.playerSheet.texture) {
        SDL_Texture* ph = createPlaceholderPlayerSheet(g.renderer);
        g.playerSheet = { ph, 16, 32, 16, 16 };
    }

    // Register animation clips on player
    registerPlayerClips(g.player.anim);

    generateZoneTrash(0);  // Beach trash

    // Animals (appear as eco improves) — Beach zone only at startup
    g.animals.clear();
    auto addAnimal = [](float x, float y, AnimalType t, float ecoReq) {
        Animal a;
        a.pos = {x, y};
        a.type = t;
        a.requiredEco = ecoReq;
        a.animPhase = randf(0, 6.28f);
        a.moveTimer = randf(1, 4);
        g.animals.push_back(a);
    };
    addAnimal(350, 680, AnimalType::CRAB, 10);
    addAnimal(600, 680, AnimalType::CRAB, 20);
    addAnimal(180, 710, AnimalType::FISH, 15);
    addAnimal(800, 500, AnimalType::BIRD, 25);

    // Clouds
    g.clouds.clear();
    for (int i = 0; i < 12; i++) {
        Cloud c;
        c.x = randf(0, (float)WORLD_W);
        c.y = randf(30, 200);
        c.w = randf(80, 200);
        c.speed = randf(8, 25);
        g.clouds.push_back(c);
    }

    // Decorations
    g.decorations.clear();
    // Beach bushes and rocks
    for (int i = 0; i < 20; i++) {
        Decoration d;
        d.pos.x = randf(300, (float)ZONE_BEACH_END - 50);
        d.pos.y = (float)getGroundY(d.pos.x);
        d.type = (i % 3 == 0) ? 2 : 1; // rocks and flowers
        d.scale = randf(0.6f, 1.2f);
        g.decorations.push_back(d);
    }
    // Forest bushes and flowers
    for (int i = 0; i < 30; i++) {
        Decoration d;
        d.pos.x = randf((float)ZONE_FOREST_START + 50, (float)ZONE_FOREST_END - 50);
        d.pos.y = (float)getGroundY(d.pos.x);
        d.type = (i % 2 == 0) ? 0 : 1;
        d.scale = randf(0.8f, 1.4f);
        g.decorations.push_back(d);
    }
    // Wetland reeds and lilies
    for (int i = 0; i < 25; i++) {
        Decoration d;
        d.pos.x = randf((float)ZONE_WETLAND_START + 50, (float)ZONE_WETLAND_END - 50);
        d.pos.y = (float)getGroundY(d.pos.x);
        d.type = (i % 2 == 0) ? 3 : 4; // reeds and lilies
        d.scale = randf(0.7f, 1.3f);
        g.decorations.push_back(d);
    }
}

// ─── Input ───────────────────────────────────────────────────────────────────

static void handleEvents() {
    // Reset single-frame inputs
    g.input.jumpPressed = false;
    g.input.actionPressed = false;
    g.input.pausePressed = false;
    g.input.enterPressed = false;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
#ifdef __EMSCRIPTEN__
                emscripten_cancel_main_loop();
#endif
                break;
            case SDL_KEYDOWN:
                if (e.key.repeat) break;
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_A:
                    case SDL_SCANCODE_LEFT:  g.input.left = true; break;
                    case SDL_SCANCODE_D:
                    case SDL_SCANCODE_RIGHT: g.input.right = true; break;
                    case SDL_SCANCODE_W:
                    case SDL_SCANCODE_UP:    g.input.up = true; break;
                    case SDL_SCANCODE_S:
                    case SDL_SCANCODE_DOWN:  g.input.down = true; break;
                    case SDL_SCANCODE_SPACE:
                        g.input.jump = true;
                        g.input.jumpPressed = true;
                        break;
                    case SDL_SCANCODE_E:
                        g.input.action = true;
                        g.input.actionPressed = true;
                        break;
                    case SDL_SCANCODE_ESCAPE:
                        g.input.pause = true;
                        g.input.pausePressed = true;
                        break;
                    case SDL_SCANCODE_RETURN:
                        g.input.enter = true;
                        g.input.enterPressed = true;
                        break;
                    default: break;
                }
                break;
            case SDL_KEYUP:
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_A:
                    case SDL_SCANCODE_LEFT:  g.input.left = false; break;
                    case SDL_SCANCODE_D:
                    case SDL_SCANCODE_RIGHT: g.input.right = false; break;
                    case SDL_SCANCODE_W:
                    case SDL_SCANCODE_UP:    g.input.up = false; break;
                    case SDL_SCANCODE_S:
                    case SDL_SCANCODE_DOWN:  g.input.down = false; break;
                    case SDL_SCANCODE_SPACE: g.input.jump = false; break;
                    case SDL_SCANCODE_E:     g.input.action = false; break;
                    case SDL_SCANCODE_ESCAPE:g.input.pause = false; break;
                    case SDL_SCANCODE_RETURN:g.input.enter = false; break;
                    default: break;
                }
                break;
        }
    }
}

// ─── Update ──────────────────────────────────────────────────────────────────

static void updateMenu(float dt) {
    g.menuAnim += dt;
    if (g.input.enterPressed || g.input.jumpPressed) {
        g.scene = GameScene::PLAYING;
        generateWorld();
        g.player.pos = {400, 500};
        g.ecoMeter = 0;
        g.targetEco = 0;
        g.currentZoneName = getZoneName(Zone::BEACH);
        g.zoneNameTimer = 3.0f;
    }
}

static void updatePlaying(float dt) {
    Player& p = g.player;
    p.animTime += dt;
    p.pickupCooldown -= dt;
    if (p.pickupCooldown < 0) p.pickupCooldown = 0;

    // ── Drive AnimationController ──────────────────────────────────────────
    {
        const std::string prevClip = p.anim.currentClip;
        if (p.pickupCooldown > 0.01f) {
            playClip(p.anim, "pickup");
        } else if (!p.onGround && !p.inWater) {
            playClip(p.anim, "jump");
        } else if (std::abs(p.vel.x) > 10.0f) {
            playClip(p.anim, "walk");
        } else {
            playClip(p.anim, "idle");
        }
        updateAnimation(p.anim, dt);
    }

    // Horizontal movement
    float moveX = 0;
    if (g.input.left)  moveX -= 1.0f;
    if (g.input.right) moveX += 1.0f;

    if (moveX != 0) {
        p.facingRight = moveX > 0;
    }

    float speed = p.inWater ? SWIM_SPEED : PLAYER_SPEED;
    p.vel.x = moveX * speed;

    // Vertical
    if (p.inWater) {
        // Swimming
        p.vel.y *= 0.92f; // water drag
        if (g.input.up)   p.vel.y -= 400.0f * dt;
        if (g.input.down)  p.vel.y += 200.0f * dt;
        p.vel.y += GRAVITY * 0.15f * dt; // slight sink
        // Clamp vertical speed in water
        if (p.vel.y > 120.0f) p.vel.y = 120.0f;
        if (p.vel.y < -180.0f) p.vel.y = -180.0f;
    } else {
        // Gravity
        p.vel.y += GRAVITY * dt;
        // Jump
        if (g.input.jumpPressed && p.onGround) {
            p.vel.y = -JUMP_FORCE;
            p.onGround = false;
        }
    }

    // Apply velocity
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;

    // World bounds — left edge hard stop
    if (p.pos.x < 20) p.pos.x = 20;

    // Zone transition (right edge) — Phase 1.5
    if (g.zoneTransitioning) {
        g.zoneTransitionTimer -= dt;
        if (g.zoneTransitionTimer <= 0) g.zoneTransitioning = false;
    }

    if (p.pos.x > WORLD_W - 32 && !g.zoneTransitioning) {
        int nextIdx = g.currentZoneIdx + 1;
        if (nextIdx < NUM_ZONES) {
            float requiredBI = ZONE_BI_THRESHOLDS[nextIdx] * 100.0f;  // in percent
            if (g.ecoMeter >= requiredBI) {
                // Unlock — switch zone
                switchZone(nextIdx);
            } else {
                // Locked — push back and flash
                p.pos.x = WORLD_W - 36;
                p.vel.x = 0;
                char buf[64];
                snprintf(buf, sizeof(buf), "Need %.0f%% ECO to enter", requiredBI);
                g.flashMsg   = buf;
                g.flashMsgTimer = 2.0f;
                g.popupMsg   = buf;
                g.popupTimer = 2.0f;
            }
        } else {
            // Last zone — hard stop at right
            p.pos.x = WORLD_W - 32;
        }
    }
    // Hard right clamp (after transition check)
    if (p.pos.x > WORLD_W - 20) p.pos.x = WORLD_W - 20;

    // CHANGED: Ground collision via TileMap (replaces getGroundY heightmap)
    p.onGround = false;
    float feetY  = p.pos.y + p.h / 2.0f;
    int   tileTs = g.currentZoneMap.tileSize;
    bool  hitGround =
        isSolidTile(g.currentZoneMap, (int)p.pos.x,               (int)feetY) ||
        isSolidTile(g.currentZoneMap, (int)(p.pos.x - p.w/2 + 2), (int)feetY) ||
        isSolidTile(g.currentZoneMap, (int)(p.pos.x + p.w/2 - 2), (int)feetY);
    if (hitGround && p.vel.y >= 0) {
        // Snap feet to top edge of the tile row they entered
        int tileRow  = (int)feetY / tileTs;
        p.pos.y = (float)(tileRow * tileTs) - p.h / 2.0f;
        p.vel.y = 0;
        p.onGround = true;
    }

    // CHANGED: Water check via TileMap (replaces isWaterAt)
    p.inWater = isWaterTile(g.currentZoneMap, (int)p.pos.x, (int)(p.pos.y));

    // Pickup trash (E key)
    if (g.input.actionPressed && p.pickupCooldown <= 0) {
        float nearestDist = 99999;
        int nearestIdx = -1;
        for (int i = 0; i < (int)g.trash.size(); i++) {
            if (g.trash[i].collected) continue;
            float dist = (g.trash[i].pos - p.pos).len();
            if (dist < PICKUP_RANGE && dist < nearestDist) {
                nearestDist = dist;
                nearestIdx = i;
            }
        }
        if (nearestIdx >= 0) {
            g.trash[nearestIdx].collected = true;
            p.trashCollected++;
            p.pickupCooldown = 0.2f;

            // Particles!
            spawnParticles(g.trash[nearestIdx].pos, {60, 200, 80, 255}, 12, 100.0f);

            // Update eco meter
            // Each zone contributes 20% of total eco; accumulate across zones
            // Simple: eco within current zone contributes fully up to the next threshold
            {
                float zoneBase  = ZONE_BI_THRESHOLDS[g.currentZoneIdx] * 100.0f;
                float zoneCap   = (g.currentZoneIdx < NUM_ZONES - 1)
                                  ? ZONE_BI_THRESHOLDS[g.currentZoneIdx + 1] * 100.0f
                                  : 100.0f;
                float zoneRange = zoneCap - zoneBase;
                float zoneProgress = (p.totalTrash > 0)
                    ? (float)p.trashCollected / (float)p.totalTrash
                    : 0.0f;
                float newEco = zoneBase + zoneProgress * zoneRange;
                if (newEco > g.targetEco) g.targetEco = newEco;  // eco never goes down
            }

            // Show popup
            const char* names[] = {"Bottle", "Can", "Plastic bag", "Tire", "Barrel"};
            char buf[64];
            snprintf(buf, sizeof(buf), "+1 %s collected! [%d/%d]",
                     names[(int)g.trash[nearestIdx].type],
                     p.trashCollected, p.totalTrash);
            g.popupMsg = buf;
            g.popupTimer = 2.0f;

            // Check win
            if (p.trashCollected >= p.totalTrash) {
                g.allTrashCollected = true;
                g.scene = GameScene::WIN;
                g.popupMsg = "Island fully restored!";
                g.popupTimer = 5.0f;
            }
        }
    }

    // Eco meter smooth animation
    if (g.ecoMeter < g.targetEco) {
        g.ecoMeter += dt * 30.0f;
        if (g.ecoMeter > g.targetEco) g.ecoMeter = g.targetEco;
    }

    // Update animals (spawn if eco is high enough, wander around)
    for (auto& a : g.animals) {
        a.animPhase += dt * 2.0f;
        if (!a.spawned && g.ecoMeter >= a.requiredEco) {
            a.spawned = true;
            spawnParticles(a.pos, {100, 255, 100, 255}, 20, 60.0f);
        }
        if (a.spawned) {
            a.moveTimer -= dt;
            if (a.moveTimer <= 0) {
                a.moveTimer = randf(2, 5);
                if (a.type == AnimalType::BIRD) {
                    a.moveDir = {randf(-40, 40), randf(-20, 10)};
                } else if (a.type == AnimalType::FISH) {
                    a.moveDir = {randf(-30, 30), randf(-10, 10)};
                } else {
                    a.moveDir = {randf(-20, 20), 0};
                }
            }
            a.pos.x += a.moveDir.x * dt;
            a.pos.y += a.moveDir.y * dt;
        }
    }

    // Update particles
    for (auto& p : g.particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 200.0f * dt;
        p.life -= dt;
    }
    g.particles.erase(
        std::remove_if(g.particles.begin(), g.particles.end(),
                        [](const Particle& p){ return p.life <= 0; }),
        g.particles.end()
    );

    // Update clouds
    for (auto& c : g.clouds) {
        c.x += c.speed * dt;
        if (c.x > WORLD_W + 200) c.x = -c.w - 50;
    }

    if (g.zoneNameTimer > 0) g.zoneNameTimer -= dt;
    if (g.flashMsgTimer > 0) g.flashMsgTimer -= dt;

    // Popup timer
    if (g.popupTimer > 0) g.popupTimer -= dt;

    // Camera follow
    float targetCamX = p.pos.x - WINDOW_W / 2.0f;
    float targetCamY = p.pos.y - WINDOW_H * 0.6f;
    g.camera.x = lerp(g.camera.x, targetCamX, 4.0f * dt);
    g.camera.y = lerp(g.camera.y, targetCamY, 4.0f * dt);

    // Clamp camera
    if (g.camera.x < 0) g.camera.x = 0;
    if (g.camera.x > WORLD_W - WINDOW_W) g.camera.x = WORLD_W - WINDOW_W;
    if (g.camera.y < 0) g.camera.y = 0;
    if (g.camera.y > WORLD_H - WINDOW_H) g.camera.y = WORLD_H - WINDOW_H;

    // Pause
    if (g.input.pausePressed) {
        g.scene = GameScene::PAUSED;
    }
}

static void updatePaused(float dt) {
    (void)dt;
    if (g.input.pausePressed || g.input.enterPressed) {
        g.scene = GameScene::PLAYING;
    }
}

static void updateWin(float dt) {
    g.menuAnim += dt;
    // Update particles still
    for (auto& p : g.particles) {
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.vel.y += 200.0f * dt;
        p.life -= dt;
    }
    g.particles.erase(
        std::remove_if(g.particles.begin(), g.particles.end(),
                        [](const Particle& p){ return p.life <= 0; }),
        g.particles.end()
    );
    // Celebratory particles
    if ((int)(g.menuAnim * 10) % 3 == 0) {
        Color cols[] = {{60,200,80,255},{80,160,255,255},{255,200,60,255}};
        spawnParticles({randf(g.camera.x, g.camera.x+WINDOW_W),
                        g.camera.y + WINDOW_H - 50},
                       cols[(int)(g.menuAnim*7)%3], 3, 150.0f);
    }

    if (g.input.enterPressed) {
        g.scene = GameScene::MENU;
    }
}

static void update(float dt) {
    switch (g.scene) {
        case GameScene::MENU:    updateMenu(dt); break;
        case GameScene::PLAYING: updatePlaying(dt); break;
        case GameScene::PAUSED:  updatePaused(dt); break;
        case GameScene::WIN:     updateWin(dt); break;
    }
}

// ─── Render ──────────────────────────────────────────────────────────────────

static void drawSky() {
    // Gradient sky
    for (int y = 0; y < WINDOW_H; y++) {
        float t = (float)y / (float)WINDOW_H;
        Color c = lerpColor(COL_SKY_TOP, COL_SKY_BOT, t);
        // Slight green tint as eco improves
        float eco01 = g.ecoMeter / 100.0f;
        c.g = (Uint8)std::min(255.0f, c.g + eco01 * 20.0f);
        setColor(c);
        SDL_RenderDrawLine(g.renderer, 0, y, WINDOW_W, y);
    }

    // Sun
    float sunX = WINDOW_W * 0.8f;
    float sunY = 80 + 20 * std::sin(g.totalTime * 0.1f);
    drawCircle((int)sunX, (int)sunY, 40, {255, 220, 80, 255});
    drawCircle((int)sunX, (int)sunY, 50, {255, 220, 80, 60});

    // Clouds (parallax)
    for (auto& c : g.clouds) {
        float parallax = 0.3f;
        float sx = c.x - g.camera.x * parallax;
        float sy = c.y;
        // Wrap
        while (sx > WINDOW_W + 200) sx -= WORLD_W + 400;
        while (sx < -300) sx += WORLD_W + 400;

        Color cc = {255, 255, 255, 80};
        drawCircle((int)sx, (int)sy, (int)(c.w*0.3f), cc);
        drawCircle((int)(sx + c.w*0.25f), (int)(sy - 5), (int)(c.w*0.35f), cc);
        drawCircle((int)(sx + c.w*0.5f), (int)sy, (int)(c.w*0.28f), cc);
    }
}

static void drawTerrain() {
    // Draw ground fill
    for (int sx = 0; sx < WINDOW_W; sx += 2) {
        float wx = sx + g.camera.x;
        int gy = getGroundY(wx);
        int sy = gy - (int)g.camera.y;

        Zone z = getZone(wx);
        Color topCol, botCol;
        switch (z) {
            case Zone::BEACH:
                topCol = COL_SAND;
                botCol = {190, 170, 130, 255};
                break;
            case Zone::FOREST:
                topCol = COL_GRASS;
                botCol = COL_DARK_GRASS;
                break;
            case Zone::WETLAND:
                topCol = COL_MUD;
                botCol = {70, 55, 35, 255};
                break;
        }

        // Ground fill
        for (int y = sy; y < WINDOW_H; y += 2) {
            float t = (float)(y - sy) / 200.0f;
            if (t > 1) t = 1;
            Color c = lerpColor(topCol, botCol, t);
            setColor(c);
            SDL_Rect r = {sx, y, 2, 2};
            SDL_RenderFillRect(g.renderer, &r);
        }

        // Grass tufts on forest
        if (z == Zone::FOREST && sx % 8 == 0) {
            int grassH = 6 + (int)(4 * std::sin(wx * 0.1f + g.totalTime));
            setColor({30, 160, 40, 200});
            SDL_RenderDrawLine(g.renderer, sx, sy, sx, sy - grassH);
            SDL_RenderDrawLine(g.renderer, sx+1, sy, sx+3, sy - grassH + 2);
        }
    }

    // Draw water
    for (int sx = 0; sx < WINDOW_W; sx += 2) {
        float wx = sx + g.camera.x;
        if (wx < 300 || wx > ZONE_WETLAND_START) {
            int wl = getWaterLevel(wx) - (int)g.camera.y;
            if (wl < WINDOW_H) {
                // Water surface wave
                float wave = 3 * std::sin(wx * 0.03f + g.totalTime * 2.0f);
                int waterTop = wl + (int)wave;
                // Semi-transparent water
                for (int y = waterTop; y < WINDOW_H; y += 2) {
                    float depth = (float)(y - waterTop) / 100.0f;
                    if (depth > 1) depth = 1;
                    Color c = lerpColor(COL_WATER, COL_WATER_DEEP, depth);
                    // Ripple effect
                    c.b = (Uint8)std::min(255.0f, c.b + 15 * std::sin(wx*0.05f + y*0.1f + g.totalTime*3));
                    setColor(c);
                    SDL_Rect r = {sx, y, 2, 2};
                    SDL_RenderFillRect(g.renderer, &r);
                }
                // Surface highlight
                setColor({180, 220, 255, 100});
                SDL_RenderDrawLine(g.renderer, sx, waterTop, sx+2, waterTop);
            }
        }
    }
}

static void drawTrees() {
    // Forest trees
    if (g.camera.x + WINDOW_W > ZONE_FOREST_START && g.camera.x < ZONE_FOREST_END) {
        for (float tx = ZONE_FOREST_START + 80; tx < ZONE_FOREST_END; tx += 120) {
            float sx = tx - g.camera.x;
            if (sx < -80 || sx > WINDOW_W + 80) continue;

            int gy = getGroundY(tx);
            float sy = gy - g.camera.y;

            // Trunk
            drawRect((int)sx - 6, (int)sy - 100, 12, 100, COL_TREE_TRUNK);

            // Canopy (layered circles)
            float sway = 3 * std::sin(g.totalTime + tx * 0.01f);
            Color leafCol = COL_TREE_LEAVES;
            // Greener with eco
            leafCol.g = (Uint8)std::min(255.0f, leafCol.g + g.ecoMeter * 0.5f);

            drawCircle((int)(sx + sway), (int)(sy - 120), 35, leafCol);
            drawCircle((int)(sx - 15 + sway*0.8f), (int)(sy - 105), 28, leafCol);
            drawCircle((int)(sx + 18 + sway*1.2f), (int)(sy - 108), 30, leafCol);
            drawCircle((int)(sx + sway*0.5f), (int)(sy - 140), 25, {20, 120, 40, 255});
        }
    }

    // Wetland reeds
    if (g.camera.x + WINDOW_W > ZONE_WETLAND_START) {
        for (float tx = ZONE_WETLAND_START + 30; tx < ZONE_WETLAND_END; tx += 40) {
            float sx = tx - g.camera.x;
            if (sx < -20 || sx > WINDOW_W + 20) continue;

            int gy = getGroundY(tx);
            float sy = gy - g.camera.y;
            float sway = 4 * std::sin(g.totalTime * 1.5f + tx * 0.02f);

            setColor({60, 120, 50, 200});
            SDL_RenderDrawLine(g.renderer, (int)sx, (int)sy, (int)(sx + sway), (int)(sy - 40));
            SDL_RenderDrawLine(g.renderer, (int)sx+2, (int)sy, (int)(sx + sway + 2), (int)(sy - 35));
        }
    }

    // Beach palm trees
    if (g.camera.x < ZONE_BEACH_END) {
        float palmPositions[] = {500, 900, 1300, 1700};
        for (float tx : palmPositions) {
            float sx = tx - g.camera.x;
            if (sx < -80 || sx > WINDOW_W + 80) continue;

            int gy = getGroundY(tx);
            float sy = gy - g.camera.y;
            float sway = 5 * std::sin(g.totalTime * 0.8f + tx * 0.005f);

            // Curved trunk
            for (int i = 0; i < 80; i++) {
                float t = i / 80.0f;
                float cx = sx + sway * t * t;
                float cy = sy - i;
                drawRect((int)cx - 4, (int)cy, 8, 2, {140, 100, 60, 255});
            }

            // Palm fronds
            float topX = sx + sway;
            float topY = sy - 80;
            for (int f = 0; f < 5; f++) {
                float angle = -2.5f + f * 1.2f;
                float fSway = sway * 0.3f;
                for (int j = 0; j < 40; j++) {
                    float ft = j / 40.0f;
                    float fx = topX + std::cos(angle + fSway*0.02f) * j * 1.5f;
                    float fy = topY + std::sin(angle) * j * 0.8f + ft*ft * 20;
                    setColor({40, 160, 50, (Uint8)(255 - ft*100)});
                    SDL_RenderDrawLine(g.renderer, (int)fx, (int)fy, (int)fx+2, (int)fy+1);
                }
            }
        }
    }
}

static void drawDecorations() {
    for (auto& d : g.decorations) {
        float sx = d.pos.x - g.camera.x;
        float sy = d.pos.y - g.camera.y;
        if (sx < -30 || sx > WINDOW_W + 30) continue;

        float s = d.scale;
        switch (d.type) {
            case 0: // bush
                drawCircle((int)sx, (int)(sy - 8*s), (int)(12*s), {50, 140, 50, 200});
                drawCircle((int)(sx + 8*s), (int)(sy - 6*s), (int)(10*s), {40, 130, 40, 200});
                break;
            case 1: // flower
            {
                setColor({30, 150, 30, 255});
                SDL_RenderDrawLine(g.renderer, (int)sx, (int)sy, (int)sx, (int)(sy - 15*s));
                Color flowerCols[] = {{255,100,100,255},{255,200,50,255},{200,100,255,255},{255,150,200,255}};
                drawCircle((int)sx, (int)(sy - 15*s), (int)(4*s), flowerCols[(int)(d.pos.x)%4]);
                break;
            }
            case 2: // rock
                drawCircle((int)sx, (int)(sy - 5*s), (int)(8*s), {130, 130, 120, 255});
                drawCircle((int)(sx + 3), (int)(sy - 7*s), (int)(5*s), {150, 150, 140, 255});
                break;
            case 3: // reed
            {
                float sway = 3 * std::sin(g.totalTime * 1.8f + d.pos.x * 0.03f);
                setColor({70, 130, 60, 220});
                SDL_RenderDrawLine(g.renderer, (int)sx, (int)sy, (int)(sx+sway), (int)(sy-30*s));
                break;
            }
            case 4: // lily pad
                drawCircle((int)sx, (int)sy, (int)(6*s), {60, 150, 60, 180});
                break;
        }
    }
}

static void drawTrash() {
    for (auto& t : g.trash) {
        if (t.collected) continue;

        float sx = t.pos.x - g.camera.x;
        float sy = t.pos.y - g.camera.y;
        if (sx < -30 || sx > WINDOW_W + 30) continue;

        // Bob in water
        if (t.inWater) {
            sy += 4 * std::sin(g.totalTime * 2 + t.bobPhase);
        }

        // Glow indicator
        float pulse = 0.5f + 0.5f * std::sin(g.totalTime * 3 + t.pos.x * 0.01f);
        drawCircle((int)sx, (int)sy, (int)(14 + pulse*4), {255, 100, 100, (Uint8)(40 + pulse*40)});

        // Draw trash based on type
        switch (t.type) {
            case TrashType::BOTTLE:
                drawRect((int)sx-3, (int)sy-10, 6, 14, COL_TRASH);
                drawRect((int)sx-2, (int)sy-13, 4, 4, {200, 80, 80, 255});
                break;
            case TrashType::CAN:
                drawRect((int)sx-4, (int)sy-8, 8, 10, {180, 180, 180, 255});
                drawRect((int)sx-4, (int)sy-8, 8, 3, COL_TRASH);
                break;
            case TrashType::BAG:
                drawCircle((int)sx, (int)sy-5, 8, {200, 200, 200, 200});
                drawCircle((int)sx+3, (int)(sy-7), 5, {180, 180, 180, 200});
                break;
            case TrashType::TIRE:
                drawCircle((int)sx, (int)sy-8, 12, {50, 50, 50, 255});
                drawCircle((int)sx, (int)sy-8, 7, {80, 70, 60, 255});
                break;
            case TrashType::BARREL:
                drawRect((int)sx-8, (int)sy-16, 16, 20, {120, 80, 40, 255});
                drawRect((int)sx-9, (int)sy-14, 18, 3, {100, 70, 30, 255});
                drawRect((int)sx-9, (int)sy-6, 18, 3, {100, 70, 30, 255});
                break;
        }

        // "E" prompt when player is near
        float dist = (t.pos - g.player.pos).len();
        if (dist < PICKUP_RANGE * 1.5f) {
            float alpha = std::max(0.0f, 1.0f - dist / (PICKUP_RANGE * 1.5f));
            drawRect((int)sx-8, (int)sy-28, 16, 14, {10, 10, 26, (Uint8)(200*alpha)});
            // E letter (crude pixel art)
            Color ec = {96, 96, 255, (Uint8)(255*alpha)};
            drawRect((int)sx-4, (int)sy-26, 8, 2, ec);
            drawRect((int)sx-4, (int)sy-22, 6, 2, ec);
            drawRect((int)sx-4, (int)sy-18, 8, 2, ec);
            drawRect((int)sx-4, (int)sy-26, 2, 10, ec);
        }
    }
}

static void drawAnimals() {
    for (auto& a : g.animals) {
        if (!a.spawned) continue;

        float sx = a.pos.x - g.camera.x;
        float sy = a.pos.y - g.camera.y;
        if (sx < -30 || sx > WINDOW_W + 30) continue;

        switch (a.type) {
            case AnimalType::CRAB:
            {
                // Orange crab body
                drawCircle((int)sx, (int)sy, 6, {220, 120, 40, 255});
                // Legs
                float legMove = std::sin(a.animPhase * 3) * 3;
                setColor({200, 100, 30, 255});
                SDL_RenderDrawLine(g.renderer, (int)sx-6, (int)sy, (int)(sx-10+legMove), (int)(sy+4));
                SDL_RenderDrawLine(g.renderer, (int)sx+6, (int)sy, (int)(sx+10-legMove), (int)(sy+4));
                // Claws
                drawCircle((int)(sx-10+legMove), (int)(sy-2), 3, {230, 130, 40, 255});
                drawCircle((int)(sx+10-legMove), (int)(sy-2), 3, {230, 130, 40, 255});
                // Eyes
                drawCircle((int)sx-2, (int)(sy-5), 2, COL_WHITE);
                drawCircle((int)sx+2, (int)(sy-5), 2, COL_WHITE);
                break;
            }
            case AnimalType::BIRD:
            {
                float flap = std::sin(a.animPhase * 4) * 8;
                // Body
                drawCircle((int)sx, (int)sy, 5, COL_BIRD);
                // Wings
                setColor(COL_BIRD);
                SDL_RenderDrawLine(g.renderer, (int)sx, (int)sy, (int)(sx-12), (int)(sy - 5 + flap));
                SDL_RenderDrawLine(g.renderer, (int)sx, (int)sy, (int)(sx+12), (int)(sy - 5 - flap));
                // Beak
                setColor({240, 180, 40, 255});
                SDL_RenderDrawLine(g.renderer, (int)sx+5, (int)sy, (int)sx+9, (int)(sy+1));
                break;
            }
            case AnimalType::DEER:
            {
                // Body
                drawCircle((int)sx, (int)sy-10, 10, COL_ANIMAL);
                drawCircle((int)sx-8, (int)sy-12, 7, COL_ANIMAL);
                // Head
                drawCircle((int)sx-14, (int)sy-18, 6, {210, 170, 90, 255});
                // Legs
                float legAnim = std::sin(a.animPhase * 2) * 3;
                setColor({170, 130, 70, 255});
                SDL_RenderDrawLine(g.renderer, (int)sx-5, (int)sy, (int)(sx-7), (int)(sy+15+legAnim));
                SDL_RenderDrawLine(g.renderer, (int)sx+5, (int)sy, (int)(sx+3), (int)(sy+15-legAnim));
                // Antlers
                setColor({160, 120, 60, 255});
                SDL_RenderDrawLine(g.renderer, (int)sx-14, (int)sy-24, (int)(sx-18), (int)(sy-32));
                SDL_RenderDrawLine(g.renderer, (int)sx-14, (int)sy-24, (int)(sx-10), (int)(sy-32));
                // Eye
                drawCircle((int)sx-16, (int)sy-18, 1, COL_BLACK);
                break;
            }
            case AnimalType::FROG:
            {
                // Body
                float hop = std::abs(std::sin(a.animPhase * 2)) * 5;
                drawCircle((int)sx, (int)(sy - hop), 7, {50, 180, 50, 255});
                // Eyes
                drawCircle((int)sx-3, (int)(sy - 6 - hop), 3, {60, 200, 60, 255});
                drawCircle((int)sx+3, (int)(sy - 6 - hop), 3, {60, 200, 60, 255});
                drawCircle((int)sx-3, (int)(sy - 6 - hop), 1, COL_BLACK);
                drawCircle((int)sx+3, (int)(sy - 6 - hop), 1, COL_BLACK);
                break;
            }
            case AnimalType::FISH:
            {
                float swim = std::sin(a.animPhase * 3) * 3;
                // Body
                setColor(COL_FISH);
                for (int i = -6; i <= 6; i++) {
                    int h = (int)(4 * std::sqrt(1.0f - (i*i)/36.0f));
                    SDL_RenderDrawLine(g.renderer, (int)(sx+i+swim), (int)(sy-h), (int)(sx+i+swim), (int)(sy+h));
                }
                // Tail
                setColor({80, 180, 160, 255});
                SDL_RenderDrawLine(g.renderer, (int)(sx+6+swim), (int)sy, (int)(sx+12+swim), (int)(sy-4));
                SDL_RenderDrawLine(g.renderer, (int)(sx+6+swim), (int)sy, (int)(sx+12+swim), (int)(sy+4));
                // Eye
                drawCircle((int)(sx-3+swim), (int)(sy-1), 1, COL_BLACK);
                break;
            }
        }
    }
}

static void drawPlayer() {
    Player& p = g.player;
    float sx = p.pos.x - g.camera.x;
    float sy = p.pos.y - g.camera.y;
    float dir = p.facingRight ? 1.0f : -1.0f;

    // ── If a sprite sheet is available, use it ────────────────────────────
    if (g.playerSheet.texture) {
        int frame = getCurrentFrameIndex(p.anim);
        // Centre the 32×64 drawn sprite on the player's logical position
        int drawX = (int)sx - 16;  // 16px × scale2 = 32px wide → offset 16
        int drawY = (int)sy - 48;  // 32px × scale2 = 64px tall → feet at sy+16
        drawFrame(g.renderer, g.playerSheet, frame, drawX, drawY,
                  !p.facingRight, 2);
        return;
    }

    // ── Fallback: SDL2 primitive drawing (no sprite sheet loaded) ─────────
    // Walking animation
    float legAnim = p.onGround && std::abs(p.vel.x) > 10 ?
                    std::sin(p.animTime * 10) * 6 : 0;
    float armAnim = legAnim * 0.7f;

    // Swimming animation
    if (p.inWater) {
        legAnim = std::sin(p.animTime * 4) * 4;
        armAnim = std::sin(p.animTime * 4 + 1.5f) * 8;
    }

    // Legs
    setColor({40, 100, 60, 255});
    SDL_RenderDrawLine(g.renderer, (int)sx, (int)(sy+8),
                       (int)(sx - 5*dir + legAnim), (int)(sy + 22));
    SDL_RenderDrawLine(g.renderer, (int)sx, (int)(sy+8),
                       (int)(sx + 5*dir - legAnim), (int)(sy + 22));

    // Body (green outfit - eco!)
    drawRect((int)sx-8, (int)sy-12, 16, 24, COL_PLAYER_BODY);

    // Arms
    setColor(COL_PLAYER_BODY);
    SDL_RenderDrawLine(g.renderer, (int)(sx - 8), (int)(sy - 4),
                       (int)(sx - 14 + armAnim), (int)(sy + 8));
    SDL_RenderDrawLine(g.renderer, (int)(sx + 8), (int)(sy - 4),
                       (int)(sx + 14 - armAnim), (int)(sy + 8));

    // Head
    drawCircle((int)sx, (int)(sy - 18), 9, COL_PLAYER_HEAD);

    // Hair
    drawRect((int)sx-8, (int)(sy-26), 16, 6, {60, 40, 20, 255});

    // Eyes
    int eyeOff = (int)(3 * dir);
    drawCircle((int)(sx + eyeOff - 2), (int)(sy - 19), 2, COL_WHITE);
    drawCircle((int)(sx + eyeOff + 2), (int)(sy - 19), 2, COL_WHITE);
    drawCircle((int)(sx + eyeOff - 2), (int)(sy - 19), 1, COL_BLACK);
    drawCircle((int)(sx + eyeOff + 2), (int)(sy - 19), 1, COL_BLACK);

    // Backpack (for collected trash)
    float bpDir = -dir;
    drawRect((int)(sx + 9*bpDir), (int)(sy - 10), 8, 16, {80, 60, 40, 255});
    drawRect((int)(sx + 10*bpDir), (int)(sy - 8), 6, 3, {100, 80, 50, 255});
}

static void drawParticles() {
    for (auto& p : g.particles) {
        float sx = p.pos.x - g.camera.x;
        float sy = p.pos.y - g.camera.y;
        float alpha = p.life / p.maxLife;
        Color c = p.col;
        c.a = (Uint8)(c.a * alpha);
        int size = (int)(3 * alpha) + 1;
        drawRect((int)sx - size/2, (int)sy - size/2, size, size, c);
    }
}

static void drawHUD() {
    // Eco Meter (top-left)
    drawRect(16, 16, 204, 28, COL_UI_BG);
    drawRect(18, 18, 200, 24, {30, 30, 50, 200});

    // Eco bar
    float eco01 = g.ecoMeter / 100.0f;
    int barW = (int)(196 * eco01);
    Color barCol;
    if (eco01 < 0.33f) barCol = lerpColor(COL_ECO_LOW, COL_ECO_MID, eco01 / 0.33f);
    else if (eco01 < 0.66f) barCol = lerpColor(COL_ECO_MID, COL_ECO_HIGH, (eco01-0.33f)/0.33f);
    else barCol = COL_ECO_HIGH;
    drawRect(20, 20, barW, 20, barCol);

    // Eco label "ECO" - pixel text
    drawRect(24, 24, 2, 12, COL_WHITE); // E
    drawRect(24, 24, 8, 2, COL_WHITE);
    drawRect(24, 29, 6, 2, COL_WHITE);
    drawRect(24, 34, 8, 2, COL_WHITE);

    drawRect(38, 24, 2, 12, COL_WHITE); // C
    drawRect(38, 24, 8, 2, COL_WHITE);
    drawRect(38, 34, 8, 2, COL_WHITE);

    drawRect(52, 24, 2, 12, COL_WHITE); // O
    drawRect(52, 24, 8, 2, COL_WHITE);
    drawRect(52, 34, 8, 2, COL_WHITE);
    drawRect(58, 24, 2, 12, COL_WHITE);

    // Trash counter (top-right area)
    int cx = WINDOW_W - 120;
    drawRect(cx - 4, 16, 108, 28, COL_UI_BG);

    // Trash icon (small bag)
    drawCircle(cx + 8, 30, 6, COL_TRASH);

    // Counter digits - crude but functional
    // Show as filled squares proportional to progress
    int collected = g.player.trashCollected;
    int total = g.player.totalTrash;
    float progress = total > 0 ? (float)collected / total : 0;
    int fillW = (int)(80 * progress);
    drawRect(cx + 20, 22, 80, 16, {30, 30, 50, 200});
    drawRect(cx + 20, 22, fillW, 16, {60, 200, 80, 200});

    // Zone name display
    if (g.zoneNameTimer > 0) {
        float alpha = std::min(1.0f, g.zoneNameTimer);
        if (g.zoneNameTimer < 0.5f) alpha = g.zoneNameTimer * 2;
        int nameW = 300;
        int nameX = (WINDOW_W - nameW) / 2;
        int nameY = 80;
        drawRect(nameX, nameY, nameW, 36, {10, 10, 26, (Uint8)(200*alpha)});
        // Zone indicator line
        drawRect(nameX, nameY + 34, nameW, 2, {96, 96, 255, (Uint8)(200*alpha)});
    }

    // Popup message
    if (g.popupTimer > 0) {
        float alpha = std::min(1.0f, g.popupTimer);
        int pw = 350;
        int px = (WINDOW_W - pw) / 2;
        int py = WINDOW_H - 100;
        drawRect(px, py, pw, 32, {10, 30, 10, (Uint8)(200*alpha)});
        drawRect(px, py, 4, 32, {60, 200, 80, (Uint8)(255*alpha)});
    }

    // Controls hint (bottom-left, fades out)
    float hintAlpha = std::max(0.0f, 1.0f - g.totalTime * 0.1f);
    if (hintAlpha > 0) {
        drawRect(16, WINDOW_H - 48, 300, 32, {10, 10, 26, (Uint8)(180*hintAlpha)});
        // Arrow/WASD hint line
        drawRect(20, WINDOW_H - 44, 2, 2, {96, 96, 255, (Uint8)(200*hintAlpha)});
        drawRect(24, WINDOW_H - 44, 30, 2, {200, 200, 240, (Uint8)(150*hintAlpha)});
    }

    // Mini-map (bottom-right)
    int mmX = WINDOW_W - 170;
    int mmY = WINDOW_H - 55;
    int mmW = 154;
    int mmH = 40;
    drawRect(mmX - 2, mmY - 2, mmW + 4, mmH + 4, COL_UI_BG);

    // Zone colors on minimap
    int beachW = (int)(mmW * (float)ZONE_BEACH_END / WORLD_W);
    int forestW = (int)(mmW * (float)(ZONE_FOREST_END - ZONE_FOREST_START) / WORLD_W);
    int wetW = mmW - beachW - forestW;
    drawRect(mmX, mmY, beachW, mmH, {220, 200, 160, 100});
    drawRect(mmX + beachW, mmY, forestW, mmH, {60, 160, 60, 100});
    drawRect(mmX + beachW + forestW, mmY, wetW, mmH, {100, 80, 50, 100});

    // Trash dots on minimap
    for (auto& t : g.trash) {
        if (t.collected) continue;
        int tx = mmX + (int)(t.pos.x / WORLD_W * mmW);
        int ty = mmY + mmH/2;
        drawRect(tx, ty, 2, 2, COL_TRASH);
    }

    // Player dot on minimap
    int plx = mmX + (int)(g.player.pos.x / WORLD_W * mmW);
    drawRect(plx - 1, mmY + mmH/2 - 1, 4, 4, COL_PLAYER_BODY);

    // Camera view indicator
    int cvx = mmX + (int)(g.camera.x / WORLD_W * mmW);
    int cvw = (int)((float)WINDOW_W / WORLD_W * mmW);
    setColor({96, 96, 255, 80});
    SDL_Rect cvr = {cvx, mmY, cvw, mmH};
    SDL_RenderFillRect(g.renderer, &cvr);
    SDL_SetRenderDrawColor(g.renderer, 96, 96, 255, 150);
    SDL_RenderDrawRect(g.renderer, &cvr);

    // DEBUG: camera.x progress bar (bottom of screen, 4px tall)
    // Shows scroll position from 0 (left) to WORLD_W-WINDOW_W (right)
    {
        const int maxScroll = WORLD_W - WINDOW_W; // 2560
        float camPct = (maxScroll > 0) ? std::min(1.0f, g.camera.x / (float)maxScroll) : 0.0f;
        int barW = (int)(camPct * WINDOW_W);
        // Dark bg
        drawRect(0, WINDOW_H - 6, WINDOW_W, 6, {10, 10, 26, 180});
        // Progress fill — green at start, yellow at mid, orange near end
        Color dbgCol = {50, 220, 100, 200};
        if (camPct > 0.66f) dbgCol = {220, 160, 50, 200};
        else if (camPct > 0.33f) dbgCol = {180, 220, 50, 200};
        drawRect(0, WINDOW_H - 6, barW, 6, dbgCol);
        // Tick at start and end
        drawRect(0, WINDOW_H - 6, 2, 6, COL_WHITE);
        drawRect(WINDOW_W - 2, WINDOW_H - 6, 2, 6, COL_WHITE);
    }
}

static void drawMenu() {
    // Background
    SDL_SetRenderDrawColor(g.renderer, 8, 8, 26, 255);
    SDL_RenderClear(g.renderer);

    // Animated background dots
    for (int y = 0; y < WINDOW_H; y += 48) {
        for (int x = 0; x < WINDOW_W; x += 48) {
            float pulse = 0.2f + 0.15f * std::sin(g.menuAnim * 1.5f + (x+y)*0.008f);
            drawRect(x + 23, y + 23, 3, 3, {60, 80, 160, (Uint8)(pulse*255)});
        }
    }

    // Title card
    int cardW = 500, cardH = 300;
    int cardX = (WINDOW_W - cardW) / 2;
    int cardY = (WINDOW_H - cardH) / 2 - 20;
    drawRect(cardX, cardY, cardW, cardH, {14, 14, 30, 230});

    // Border
    float bright = 0.5f + 0.5f * std::sin(g.menuAnim * 2);
    Uint8 bv = (Uint8)(bright * 160 + 40);
    setColor({bv, (Uint8)(bv/2), 255, 255});
    SDL_Rect border = {cardX, cardY, cardW, cardH};
    SDL_RenderDrawRect(g.renderer, &border);

    // Title area - "ZIELONY PŁYWAK" in green
    drawRect(cardX + 20, cardY + 30, cardW - 40, 4, {60, 200, 80, 200});
    drawRect(cardX + 20, cardY + 80, cardW - 40, 2, {60, 200, 80, 100});

    // Subtitle area
    drawRect(cardX + 20, cardY + 95, cardW - 40, 2, {96, 96, 255, 60});

    // Leaf decoration
    for (int i = 0; i < 5; i++) {
        float lx = cardX + 60 + i * 90;
        float ly = cardY + 55;
        float sway = 3 * std::sin(g.menuAnim * 2 + i);
        drawCircle((int)(lx + sway), (int)ly, 8, {40, 160, 50, 150});
    }

    // "Press ENTER to start" indicator
    float blink = 0.5f + 0.5f * std::sin(g.menuAnim * 3);
    drawRect(cardX + 120, cardY + 220, cardW - 240, 30,
             {96, 96, 255, (Uint8)(60 + blink*100)});
    drawRect(cardX + 122, cardY + 222, cardW - 244, 26,
             {14, 14, 30, 200});

    // Footer
    drawRect(0, WINDOW_H - 28, WINDOW_W, 28, {8, 8, 26, 200});
    drawRect(0, WINDOW_H - 28, WINDOW_W, 1, {96, 96, 255, 40});
}

static void drawPaused() {
    // Dim overlay
    drawRect(0, 0, WINDOW_W, WINDOW_H, {0, 0, 0, 150});

    // Pause card
    int pw = 300, ph = 150;
    int px = (WINDOW_W - pw) / 2;
    int py = (WINDOW_H - ph) / 2;
    drawRect(px, py, pw, ph, {14, 14, 30, 240});
    setColor(COL_UI_ACCENT);
    SDL_Rect b = {px, py, pw, ph};
    SDL_RenderDrawRect(g.renderer, &b);

    // Pause bars
    drawRect(px + pw/2 - 20, py + 40, 15, 50, COL_UI_ACCENT);
    drawRect(px + pw/2 + 5, py + 40, 15, 50, COL_UI_ACCENT);

    // "ESC to resume" hint
    drawRect(px + 60, py + ph - 30, pw - 120, 20, {96, 96, 255, 60});
}

static void drawWin() {
    // Render the game world behind
    drawSky();
    // CHANGED: TileMap renderer
    drawTileMap(g.renderer, g.currentZoneMap, (int)g.camera.x, (int)g.camera.y);
    drawTrees();
    drawDecorations();
    drawAnimals();
    drawPlayer();
    drawParticles();

    // Green overlay
    drawRect(0, 0, WINDOW_W, WINDOW_H, {20, 60, 20, 120});

    // Win card
    int cw = 500, ch = 250;
    int cx = (WINDOW_W - cw) / 2;
    int cy = (WINDOW_H - ch) / 2;
    drawRect(cx, cy, cw, ch, {14, 30, 14, 230});

    // Green border
    float pulse = 0.5f + 0.5f * std::sin(g.menuAnim * 3);
    setColor({60, (Uint8)(160 + pulse*95), 60, 255});
    SDL_Rect b = {cx, cy, cw, ch};
    SDL_RenderDrawRect(g.renderer, &b);

    // Eco 100% full bar
    drawRect(cx + 30, cy + 60, cw - 60, 20, {30, 50, 30, 200});
    drawRect(cx + 30, cy + 60, cw - 60, 20, COL_ECO_HIGH);

    // Star decorations
    for (int i = 0; i < 5; i++) {
        float sx = cx + 100 + i * 70;
        float sy = cy + 120;
        float twinkle = 0.5f + 0.5f * std::sin(g.menuAnim * 4 + i * 1.2f);
        int size = (int)(6 + twinkle * 4);
        drawCircle((int)sx, (int)sy, size, {255, 220, 60, (Uint8)(200 + twinkle*55)});
    }

    // "ENTER to return" blink
    float blink = 0.5f + 0.5f * std::sin(g.menuAnim * 3);
    drawRect(cx + 120, cy + 190, cw - 240, 30,
             {60, 200, 80, (Uint8)(60 + blink*120)});
}

static void drawZoneGate() {
    if (g.currentZoneIdx >= NUM_ZONES - 1) return;  // last zone, no gate

    float nextThreshold = ZONE_BI_THRESHOLDS[g.currentZoneIdx + 1] * 100.0f;
    bool  unlocked = g.ecoMeter >= nextThreshold;
    Color gateCol  = unlocked ? COL_GATE_UNLOCKED : COL_GATE_LOCKED;

    int gateWorldX = WORLD_W - 32;
    int gateScreenX = gateWorldX - (int)g.camera.x;

    if (gateScreenX < -8 || gateScreenX > WINDOW_W + 8) return;

    // Dashed vertical line
    for (int y = 0; y < WINDOW_H; y += 18) {
        drawRect(gateScreenX, y, 5, 12, gateCol);
    }

    // Small arrow + hint at mid-screen height
    int midY = WINDOW_H / 2 - 40;
    drawRect(gateScreenX - 20, midY, 50, 30, {10, 10, 26, 200});
    // Arrow pointing right (3 rects)
    Color arrowCol = unlocked ? COL_GATE_UNLOCKED : COL_GATE_LOCKED;
    drawRect(gateScreenX - 14, midY + 13, 20, 4, arrowCol);  // shaft
    drawRect(gateScreenX + 5,  midY + 8,  6, 14, arrowCol); // arrowhead right bar
    // Lock icon if locked (simple X)
    if (!unlocked) {
        drawRect(gateScreenX - 12, midY + 5,  4, 4, {220, 80, 80, 255});
        drawRect(gateScreenX - 12, midY + 18, 4, 4, {220, 80, 80, 255});
    }
}

static void render() {
    switch (g.scene) {
        case GameScene::MENU:
            drawMenu();
            break;
        case GameScene::PLAYING:
            drawSky();
            // CHANGED: TileMap renderer replaces procedural drawTerrain()
            drawTileMap(g.renderer, g.currentZoneMap, (int)g.camera.x, (int)g.camera.y);
            drawTrees();
            drawDecorations();
            drawTrash();
            drawAnimals();
            drawPlayer();
            drawParticles();
            drawHUD();
            drawZoneGate();
            break;
        case GameScene::PAUSED:
            drawSky();
            // CHANGED: TileMap renderer replaces procedural drawTerrain()
            drawTileMap(g.renderer, g.currentZoneMap, (int)g.camera.x, (int)g.camera.y);
            drawTrees();
            drawDecorations();
            drawTrash();
            drawAnimals();
            drawPlayer();
            drawParticles();
            drawHUD();
            drawPaused();
            break;
        case GameScene::WIN:
            drawWin();
            break;
    }

    SDL_RenderPresent(g.renderer);
}

// ─── Main loop ───────────────────────────────────────────────────────────────

void mainLoopCallback() {
    Uint32 now = SDL_GetTicks();
    g.deltaTime = (now - g.lastTick) / 1000.0f;
    if (g.deltaTime > 0.05f) g.deltaTime = 0.05f;
    g.lastTick = now;
    g.totalTime += g.deltaTime;
    g.frameCount++;

    handleEvents();
    update(g.deltaTime);
    render();
}

// ─── Entry Point ─────────────────────────────────────────────────────────────

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    g.window = SDL_CreateWindow(
        "Zielony Plywak",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN
    );
    if (!g.window) { SDL_Log("CreateWindow: %s", SDL_GetError()); return 1; }

    g.renderer = SDL_CreateRenderer(
        g.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!g.renderer) { SDL_Log("CreateRenderer: %s", SDL_GetError()); return 1; }

    SDL_SetRenderDrawBlendMode(g.renderer, SDL_BLENDMODE_BLEND);
    g.lastTick = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoopCallback, 0, 1);
#else
    bool running = true;
    while (running) {
        mainLoopCallback();
        SDL_Delay(1);
    }
#endif

    SDL_DestroyRenderer(g.renderer);
    SDL_DestroyWindow(g.window);
    SDL_Quit();
    return 0;
}
