/**
 * Portfolio Game - Template
 * C++ / SDL2 / WebAssembly
 *
 * This is the entry point and game loop skeleton.
 * Game logic, scenes and content are intentionally left empty.
 */

#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstdio>
#include <string>

// ─── Constants ───────────────────────────────────────────────────────────────

constexpr int   WINDOW_W  = 1280;
constexpr int   WINDOW_H  = 720;
constexpr float TARGET_FPS = 60.0f;
const char*     WINDOW_TITLE = "Portfolio – Loading...";

// ─── Game State ──────────────────────────────────────────────────────────────

struct GameState
{
    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;

    // Timing
    Uint32  lastTick  = 0;
    float   deltaTime = 0.0f;   // seconds since last frame
    float   totalTime = 0.0f;   // seconds since start
    Uint64  frameCount = 0;

    // Input snapshot (filled each frame)
    struct Input
    {
        int  mouseX = 0, mouseY = 0;
        bool mouseDown = false;
        const Uint8* keys = nullptr;   // SDL keyboard state array
    } input;

    // TODO: add scene/state machine here
};

static GameState g;  // single global instance

// ─── Forward declarations ─────────────────────────────────────────────────────

void handleEvents();
void update(float dt);
void render();
void mainLoopCallback();   // called by Emscripten each frame

// ─── Input handling ──────────────────────────────────────────────────────────

void handleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
            case SDL_QUIT:
                emscripten_cancel_main_loop();
                break;

            // TODO: forward events to UI / scene manager
            default:
                break;
        }
    }

    // Refresh per-frame input state
    int mx, my;
    Uint32 mb       = SDL_GetMouseState(&mx, &my);
    g.input.mouseX  = mx;
    g.input.mouseY  = my;
    g.input.mouseDown = (mb & SDL_BUTTON(1)) != 0;
    g.input.keys    = SDL_GetKeyboardState(nullptr);
}

// ─── Update ──────────────────────────────────────────────────────────────────

void update(float dt)
{
    (void)dt;
    // TODO: tick scene / entities / animations
}

// ─── Render ──────────────────────────────────────────────────────────────────

// A tiny helper: draw a filled rectangle
static void drawRect(int x, int y, int w, int h, Uint8 r, Uint8 g2, Uint8 b, Uint8 a = 255)
{
    SDL_SetRenderDrawBlendMode(g.renderer, a < 255 ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(g.renderer, r, g2, b, a);
    SDL_Rect rect{ x, y, w, h };
    SDL_RenderFillRect(g.renderer, &rect);
}

void render()
{
    // ── Background ──────────────────────────────────────────────────────────
    SDL_SetRenderDrawColor(g.renderer, 8, 8, 18, 255);
    SDL_RenderClear(g.renderer);

    // ── Animated placeholder grid ───────────────────────────────────────────
    // Subtle scrolling dots to confirm the loop is alive
    const int GRID = 48;
    float scroll = std::fmod(g.totalTime * 24.0f, (float)GRID);
    for (int y = -(int)scroll; y < WINDOW_H + GRID; y += GRID)
    {
        for (int x = -(int)scroll; x < WINDOW_W + GRID; x += GRID)
        {
            float pulse = 0.25f + 0.15f * std::sin(g.totalTime * 1.5f + (x + y) * 0.01f);
            Uint8 alpha = (Uint8)(pulse * 255.0f);
            drawRect(x + GRID / 2 - 1, y + GRID / 2 - 1, 3, 3, 60, 80, 160, alpha);
        }
    }

    // ── Centre placeholder card ─────────────────────────────────────────────
    const int CW = 480, CH = 200;
    const int CX = (WINDOW_W - CW) / 2;
    const int CY = (WINDOW_H - CH) / 2;

    // Card background
    drawRect(CX, CY, CW, CH, 14, 14, 30, 220);

    // Accent border (pulsing)
    float bright = 0.5f + 0.5f * std::sin(g.totalTime * 2.0f);
    Uint8 bv = (Uint8)(bright * 160.0f + 40.0f);
    SDL_SetRenderDrawColor(g.renderer, bv, bv / 2, 255, 255);
    SDL_Rect border{ CX, CY, CW, CH };
    SDL_RenderDrawRect(g.renderer, &border);

    // ── Crosshair on mouse position ─────────────────────────────────────────
    int mx = g.input.mouseX, my = g.input.mouseY;
    Uint8 cv = g.input.mouseDown ? 255 : 160;
    SDL_SetRenderDrawColor(g.renderer, cv, cv, cv, 180);
    SDL_RenderDrawLine(g.renderer, mx - 10, my, mx + 10, my);
    SDL_RenderDrawLine(g.renderer, mx, my - 10, mx, my + 10);

    // ── Present ─────────────────────────────────────────────────────────────
    SDL_RenderPresent(g.renderer);
}

// ─── Main loop callback (called by Emscripten) ───────────────────────────────

void mainLoopCallback()
{
    // Timing
    Uint32 now    = SDL_GetTicks();
    g.deltaTime   = (now - g.lastTick) / 1000.0f;
    g.lastTick    = now;
    g.totalTime  += g.deltaTime;
    g.frameCount++;

    handleEvents();
    update(g.deltaTime);
    render();
}

// ─── Entry point ─────────────────────────────────────────────────────────────

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    g.window = SDL_CreateWindow(
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN
    );
    if (!g.window) { SDL_Log("CreateWindow failed: %s", SDL_GetError()); return 1; }

    g.renderer = SDL_CreateRenderer(
        g.window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!g.renderer) { SDL_Log("CreateRenderer failed: %s", SDL_GetError()); return 1; }

    g.lastTick = SDL_GetTicks();

    // Hand control to Emscripten's event loop (0 = use requestAnimationFrame)
    emscripten_set_main_loop(mainLoopCallback, 0, 1);

    // Cleanup (only reached if loop is cancelled)
    SDL_DestroyRenderer(g.renderer);
    SDL_DestroyWindow(g.window);
    SDL_Quit();
    return 0;
}
