#pragma once
// Phase 4.4 — Wildlife Photo Atlas
#include <SDL2/SDL.h>
#include <string>
#include <vector>

// One entry per photographable species
enum class SpeciesID {
    CRAB = 0,
    BIRD,
    DEER,
    FROG,
    FISH,
    EAGLE,
    SEA_TURTLE,
    HARE,
    WOODPECKER,
    LICHEN,
    COUNT
};

static const char* SPECIES_NAMES[(int)SpeciesID::COUNT] = {
    "Shore Crab", "Seagull", "Roe Deer", "Marsh Frog", "Green Goby",
    "White-tailed Eagle", "Sea Turtle", "European Hare", "Great Spotted Woodpecker", "Map Lichen"
};

// Zone where each species lives (0=Beach,1=Shallows,2=Meadow,3=Forest,4=Hill)
static const int SPECIES_ZONE[(int)SpeciesID::COUNT] = {
    0, 0, 3, 1, 1, 4, 1, 2, 3, 4
};

struct AtlasEntry {
    SpeciesID id;
    bool photographed = false;
};

struct WildlifeAtlas {
    AtlasEntry entries[(int)SpeciesID::COUNT];
    bool uiOpen = false;   // [Tab] toggles atlas screen
    int  scrollPos = 0;    // which row is at top

    WildlifeAtlas() {
        for (int i = 0; i < (int)SpeciesID::COUNT; i++) {
            entries[i].id = (SpeciesID)i;
        }
    }

    void photograph(SpeciesID id) {
        entries[(int)id].photographed = true;
    }

    int countPhotographed() const {
        int n = 0;
        for (auto& e : entries) if (e.photographed) n++;
        return n;
    }

    // Render the atlas full-screen overlay
    void render(SDL_Renderer* ren, int winW, int winH) const {
        if (!uiOpen) return;
        // Dark overlay
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 6, 10, 20, 230);
        SDL_Rect bg = {0, 0, winW, winH};
        SDL_RenderFillRect(ren, &bg);
        // Title bar
        SDL_SetRenderDrawColor(ren, 20, 30, 50, 255);
        SDL_Rect titleBg = {0, 0, winW, 44};
        SDL_RenderFillRect(ren, &titleBg);
        SDL_SetRenderDrawColor(ren, 60, 200, 80, 255);
        SDL_Rect titleLine = {0, 42, winW, 2};
        SDL_RenderFillRect(ren, &titleLine);

        // Stats
        char stats[64];
        snprintf(stats, sizeof(stats), "WILDLIFE ATLAS  %d / %d", countPhotographed(), (int)SpeciesID::COUNT);
        (void)stats; // rendered as green bar indicator below

        // Progress bar
        int barW = winW - 80;
        SDL_SetRenderDrawColor(ren, 30, 40, 30, 200);
        SDL_Rect pb = {40, 10, barW, 22};
        SDL_RenderFillRect(ren, &pb);
        int fillW = (int)(barW * (float)countPhotographed() / (float)(int)SpeciesID::COUNT);
        SDL_SetRenderDrawColor(ren, 60, 200, 80, 220);
        SDL_Rect pf = {40, 10, fillW, 22};
        SDL_RenderFillRect(ren, &pf);

        // Species grid (2 columns)
        int startY = 56;
        int colW = winW / 2 - 20;
        for (int i = 0; i < (int)SpeciesID::COUNT; i++) {
            int col = i % 2;
            int row = i / 2;
            int ex = 20 + col * (colW + 20);
            int ey = startY + row * 60;
            bool photo = entries[i].photographed;

            // Entry background
            SDL_SetRenderDrawColor(ren, photo ? 20 : 10, photo ? 40 : 20, photo ? 30 : 20, 200);
            SDL_Rect eb = {ex, ey, colW, 52};
            SDL_RenderFillRect(ren, &eb);
            // Border
            SDL_SetRenderDrawColor(ren, photo ? 60 : 40, photo ? 200 : 80, photo ? 80 : 60, 200);
            SDL_RenderDrawRect(ren, &eb);

            // Species colour dot
            struct { Uint8 r, g, b; } dotCols[] = {
                {220,120,40},{80,140,200},{200,160,80},{50,180,50},{100,200,180},
                {80,140,200},{100,200,180},{160,200,80},{80,140,200},{150,180,130}
            };
            SDL_SetRenderDrawColor(ren, dotCols[i].r, dotCols[i].g, dotCols[i].b, photo ? 255 : 60);
            SDL_Rect dot = {ex + 8, ey + 14, 24, 24};
            SDL_RenderFillRect(ren, &dot);

            // Zone badge
            SDL_SetRenderDrawColor(ren, 40, 60, 80, 180);
            SDL_Rect zb = {ex + colW - 60, ey + 6, 54, 16};
            SDL_RenderFillRect(ren, &zb);
            // zone colour strip
            struct { Uint8 r, g, b; } zoneCols[] = {
                {220,200,160},{40,80,180},{60,160,60},{30,100,40},{120,110,100}
            };
            int z = SPECIES_ZONE[i];
            SDL_SetRenderDrawColor(ren, zoneCols[z].r, zoneCols[z].g, zoneCols[z].b, 200);
            SDL_Rect zs = {ex + colW - 58, ey + 8, 50, 12};
            SDL_RenderFillRect(ren, &zs);

            // Photo tick if photographed
            if (photo) {
                SDL_SetRenderDrawColor(ren, 60, 255, 80, 255);
                SDL_RenderDrawLine(ren, ex + colW - 20, ey + 18, ex + colW - 15, ey + 24);
                SDL_RenderDrawLine(ren, ex + colW - 15, ey + 24, ex + colW - 6, ey + 12);
            }
        }

        // Footer hint
        SDL_SetRenderDrawColor(ren, 20, 30, 50, 255);
        SDL_Rect foot = {0, winH - 30, winW, 30};
        SDL_RenderFillRect(ren, &foot);
        SDL_SetRenderDrawColor(ren, 60, 200, 80, 80);
        SDL_Rect footLine = {0, winH - 30, winW, 1};
        SDL_RenderFillRect(ren, &footLine);
    }
};
