# Zielony Pływak — Step-by-Step Implementation Prompts
> One prompt per task. Paste into your AI coding assistant (Cursor, Copilot, Claude, etc.)  
> Context: C++ · SDL2 · Emscripten/WASM · No external assets until Phase 6  
> Codebase entry point: `src/main.cpp` (~1100 lines, all primitives)

---

## PHASE 0 — Foundation & Cleanup

---

### 0.1.1 — Define tile map data structure

```
We are building a 2D side-scroller in C++ with SDL2, compiled to WebAssembly via Emscripten.
The current codebase uses runtime procedural terrain generation. We need to replace it with
a hand-crafted tilemap system.

Define a TileMap data structure in a new file `src/zone.h`. Requirements:
- A Tile struct with: int tileId, bool solid, bool water
- A TileMap struct with: 2D array (or vector<vector<Tile>>) of tiles, int widthInTiles,
  int heightInTiles, int tileSize (default 16px)
- A function signature: TileMap loadTileMap(const std::string& data) that accepts a
  multiline string of integers (0 = air, 1 = solid ground, 2 = water surface, 3 = deep water)
- A function signature: void drawTileMap(SDL_Renderer* renderer, const TileMap& map,
  int cameraX, int cameraY) that draws only tiles visible in the current viewport
  (1280x720 window) using SDL_FillRect with placeholder colours per tile type.

Do not implement graphics yet — coloured rectangles per tile type are sufficient.
The camera offset (cameraX, cameraY) should cull tiles outside the viewport.
```

---

### 0.1.2 — Draw Beach zone tilemap

```
In our C++/SDL2 game we have a TileMap struct (int tileId, bool solid, bool water).
Tile IDs: 0=air, 1=sand ground, 2=water surface, 3=deep water, 4=rock, 5=platform.

Define a hardcoded Beach zone tilemap as a multiline string constant in `src/zones/beach.h`.
Requirements:
- 240 tiles wide × 45 tiles tall (240×16 = 3840px wide, 45×16 = 720px tall — exactly
  1 screen tall, 15 screens wide at 256px each)
- Terrain shape: flat sandy ground occupying bottom 8 rows, occasional rocks (tileId 4),
  shallow water pool in the middle third (tileId 2/3), elevated sand platform on right side
- The data should be a raw multiline string of space-separated integers, one row per line
- Provide a const char* BEACH_MAP string and a function TileMap buildBeachZone() that
  calls loadTileMap(BEACH_MAP) and returns the result

Keep it simple — no decorations yet. This is just the collision and terrain layer.
```

---

### 0.1.3 — Replace procedural terrain gen with tilemap renderer

```
In our C++/SDL2 game, the current main.cpp generates terrain procedurally at runtime
(sinusoidal heightmap, random platforms). We need to replace this with our new TileMap system.

In main.cpp:
1. Remove all procedural terrain generation code (the heightmap array, platform generation loop)
2. Include "zones/beach.h" and call buildBeachZone() during game init to get a TileMap
3. Store it as a global or Game struct member: TileMap currentZone
4. In the render loop, replace the old terrain draw calls with drawTileMap(renderer,
   currentZone, camera.x, camera.y)
5. In the physics/collision loop, replace heightmap collision checks with a new function:
   bool isSolidTile(const TileMap& map, int pixelX, int pixelY) that looks up the tile
   at those pixel coordinates and returns tile.solid

Show only the changed sections of main.cpp, clearly marked with // CHANGED and // REMOVED comments.
```

---

### 0.1.4 — Verify camera scroll across full zone width

```
Our C++/SDL2 game now uses a TileMap that is 3840px wide (240 tiles × 16px).
The viewport is 1280×720. The player can walk right and the camera should follow.

Implement and verify camera scrolling:
1. Add a Camera struct: int x, y with clamp logic so x stays in [0, mapWidth - viewportWidth]
2. In the update loop, set camera.x = clamp(player.x - 640, 0, 3840 - 1280) to keep
   player horizontally centred
3. All world-space draw calls (tilemap, entities, litter objects) must subtract camera.x/y
   from their screen position
4. Add a debug overlay (small text in top-left corner) showing current camera.x value
   so we can verify it reaches 0 on the left and 2560 on the right

The player should be able to walk from x=0 to x=3840 with the camera smoothly following.
Flag any draw calls in the current codebase that are missing the camera offset subtraction.
```

---

### 0.2.1 — Agree on sprite sheet format and write loader

```
We are building a C++/SDL2/Emscripten game. We need a sprite sheet loader.

Create `src/spritesheet.h` and `src/spritesheet.cpp` with:

1. A SpriteSheet struct:
   - SDL_Texture* texture
   - int frameWidth, frameHeight
   - int columns (how many frames per row in the sheet)

2. SpriteSheet loadSpriteSheet(SDL_Renderer* renderer, const char* path,
   int frameWidth, int frameHeight)
   - Loads a PNG file from path using SDL_image (IMG_LoadTexture)
   - Calculates columns = textureWidth / frameWidth
   - Returns the struct

3. void drawFrame(SDL_Renderer* renderer, const SpriteSheet& sheet,
   int frameIndex, int destX, int destY, bool flipH)
   - Calculates source rect: col = frameIndex % columns, row = frameIndex / columns
   - src = { col*frameWidth, row*frameHeight, frameWidth, frameHeight }
   - dest = { destX, destY, frameWidth*2, frameHeight*2 } (2× scale for 16px → 32px on screen)
   - Calls SDL_RenderCopyEx with flip flag

4. Add -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' to the Emscripten compile flags
   in the Makefile

For now the PNG files don't exist — the loader should print a warning and return a null
texture if the file is missing, without crashing.
```

---

### 0.2.2 — Write animation system

```
In our C++/SDL2 game we have a SpriteSheet loader (loadSpriteSheet, drawFrame).
Now we need a per-entity animation system.

Create an Animation struct and AnimationController in `src/animation.h`:

1. AnimationClip struct:
   - int startFrame, frameCount
   - float frameDuration (seconds per frame)
   - bool loop

2. AnimationController struct:
   - std::map<std::string, AnimationClip> clips
   - std::string currentClip
   - int currentFrame
   - float timer

3. Functions:
   - void addClip(AnimationController& ac, const std::string& name, AnimationClip clip)
   - void playClip(AnimationController& ac, const std::string& name) — only resets if
     switching to a different clip
   - void updateAnimation(AnimationController& ac, float deltaTime) — advances timer,
     increments currentFrame, handles loop vs stop-on-last-frame
   - int getCurrentFrameIndex(const AnimationController& ac) — returns
     clip.startFrame + ac.currentFrame for use with drawFrame()

Player clips to define:
- "idle":   startFrame=0,  frameCount=4, frameDuration=0.2s, loop=true
- "walk":   startFrame=4,  frameCount=6, frameDuration=0.1s, loop=true
- "jump":   startFrame=10, frameCount=3, frameDuration=0.15s, loop=false
- "pickup": startFrame=13, frameCount=3, frameDuration=0.12s, loop=false
```

---

### 0.2.3 — Test with placeholder player sprite

```
In our C++/SDL2 game we have an AnimationController and SpriteSheet system.
We don't have real art yet, so we need a placeholder sprite sheet generated at runtime.

Write a function SDL_Texture* createPlaceholderPlayerSheet(SDL_Renderer* renderer) that:
1. Creates an SDL_Texture of size 256×32 (16 frames × 16px wide, 1 row × 32px tall)
   using SDL_CreateTexture with SDL_TEXTUREACCESS_TARGET
2. Sets it as the render target (SDL_SetRenderTarget)
3. Draws each of the 16 frames (0–15) as a 16×32 rectangle with:
   - A green body (rect filling most of the frame)
   - A slightly different shade or small white dot per frame to distinguish animation frames
   - Frame number drawn as a 1-digit indicator (use SDL_SetRenderDrawColor variations)
4. Resets render target to NULL
5. Returns the texture

Then wire it up: in Game::init(), if loadSpriteSheet("assets/player.png") returns null texture,
call createPlaceholderPlayerSheet() and use that instead.

The player should visibly animate through walk/idle cycles using the placeholder sheet.
```

---

### 0.3.1 — Split main.cpp into modules

```
Our C++/SDL2 game has grown to ~1100 lines in a single main.cpp. We need to split it into
focused modules. Do NOT change any logic — only move code.

Create these files:
- src/game.h / game.cpp — Game struct, init(), update(), render(), cleanup()
- src/player.h / player.cpp — Player struct, updatePlayer(), renderPlayer()
- src/zone.h / zone.cpp — Zone struct, TileMap, loadZone(), renderZone()
- src/hud.h / hud.cpp — renderHUD(), renderMinimap(), renderEcoBar()
- src/entity.h / entity.cpp — Entity base struct (pos, sprite, active), updateEntity(), renderEntity()
- src/main.cpp — only: SDL init, window/renderer creation, main loop calling game methods, SDL_Quit

Rules:
- Use include guards (#pragma once) in all headers
- Forward-declare SDL types where possible to avoid heavy includes in headers
- Game struct holds: Player player, Zone currentZone, float biodiversityIndex, int credits
- Pass SDL_Renderer* into all render functions — no globals for renderer
- Show the complete content of each new file
```

---

### 0.3.2 — Define Zone struct

```
In `src/zone.h` for our C++/SDL2 game, define a complete Zone struct. Requirements:

struct LitterObject {
    int x, y;           // world-space pixel position
    LitterType type;    // PLASTIC, ORGANIC, CHEMICAL, MACHINERY
    bool collected;
    bool visible;       // false = hidden until Baba Sela favour
};

struct WildlifeSlot {
    std::string speciesName;
    int x, y;
    bool spawned;       // true once BI threshold is reached
    bool photographed;
};

struct Zone {
    std::string name;
    TileMap tilemap;
    std::vector<LitterObject> litter;
    std::vector<WildlifeSlot> wildlife;
    float biUnlockThreshold;  // 0.0–1.0, BI required to enter
    float colourSaturation;   // 0.0 = greyscale, 1.0 = full colour
    int recycleStationX, recycleStationY;  // world-space position
};

Also define:
enum class LitterType { PLASTIC, ORGANIC, CHEMICAL, MACHINERY };

And a factory function Zone createBeachZone() that populates a Zone with:
- Name "Beach"
- biUnlockThreshold = 0.0 (always accessible)
- 12 LitterObjects of mixed types at hardcoded positions
- 2 WildlifeSlots: Seagull and Crab, both spawned=false
- RecycleStation at x=200, y=600
```

---

### 0.3.3 — Define Entity base class

```
In `src/entity.h` for our C++/SDL2 game, define an Entity base struct and a component system.

struct Entity {
    float x, y;           // world-space position
    float velX, velY;     // velocity in pixels/second
    int width, height;    // bounding box
    bool active;          // inactive entities skip update + render
    SpriteSheet* sheet;   // nullable — falls back to coloured rect
    AnimationController anim;
};

Derived structs (not full classes, just structs with type tags):

struct PlayerEntity : Entity {
    bool onGround;
    bool inWater;
    float stamina;        // 0.0–1.0
    int carryCount;
    bool hasBurden;       // true when carryCount > 10
};

struct NPCEntity : Entity {
    std::string npcId;    // "marta", "tomek", "sela"
    bool regardGiven;
    bool favourComplete;
};

struct AnimalEntity : Entity {
    std::string speciesName;
    float wanderTimer;
    float wanderDirX;
};

Write a generic void renderEntity(SDL_Renderer* r, const Entity& e, int camX, int camY)
that: if sheet != nullptr draws the current animation frame, else draws a coloured
rectangle (green for player, yellow for NPC, cyan for animal) at screen position
(e.x - camX, e.y - camY).
```

---

### 0.4.1 — Add SDL2_mixer to build

```
We are building a C++/SDL2/Emscripten game. Add SDL2_mixer audio support to the build.

1. In the Makefile, add to EMFLAGS:
   -s USE_SDL_MIXER=2
   -s SDL2_MIXER_FORMATS='["ogg","mp3"]'
   --preload-file assets/audio

2. Create the directory structure: assets/audio/sfx/ and assets/audio/music/

3. Create a placeholder silence .ogg file script (shell command using ffmpeg):
   ffmpeg -f lavfi -i anullsrc=r=44100:cl=mono -t 1 -q:a 9 -acodec libvorbis assets/audio/sfx/placeholder.ogg

4. In the Makefile, add a rule `assets` that generates all placeholder audio files
   by copying placeholder.ogg to each required slot:
   sfx_pickup.ogg, sfx_recycle.ogg, sfx_compost.ogg, sfx_zonegate.ogg, music_base.ogg

Show the full updated Makefile.
```

---

### 0.4.2 — Write AudioManager

```
In our C++/SDL2 game, create `src/audio.h` and `src/audio.cpp` with an AudioManager.

Requirements:
- Use SDL2_mixer (Mix_OpenAudio, Mix_LoadMUS, Mix_LoadWAV)
- AudioManager struct with:
  - std::map<std::string, Mix_Chunk*> sfx
  - std::map<std::string, Mix_Music*> music
  - float masterVolume (0.0–1.0)
  - std::string currentMusic

- Functions:
  void initAudio()  — Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)
  void loadSFX(const std::string& id, const char* path)
  void loadMusic(const std::string& id, const char* path)
  void playSFX(const std::string& id)   — Mix_PlayChannel(-1, sfx[id], 0)
  void playMusic(const std::string& id, bool loop)  — Mix_PlayMusic, skips if already playing
  void stopMusic()
  void setVolume(float v)  — sets MIX_MAX_VOLUME * v for all channels
  void cleanupAudio()  — Mix_FreeChunk/Music for all loaded assets, Mix_CloseAudio

- In Game::init(), call initAudio(), then load placeholders:
  loadSFX("pickup", "assets/audio/sfx/sfx_pickup.ogg")
  loadSFX("recycle", "assets/audio/sfx/sfx_recycle.ogg")
  loadMusic("base", "assets/audio/music/music_base.ogg")
  playMusic("base", true)

Guard all Mix_ calls with null checks and print warnings on failure.
```

---

## PHASE 1 — All 5 Zones Blocked Out

---

### 1.1.1 — Zone 2: Shallows tilemap

```
In our C++/SDL2 game, create `src/zones/shallows.h` with a Zone createShallowsZone() function.

Tilemap spec (same format as Beach — space-separated ints, 240×45 tiles, tileId meanings:
0=air, 1=ground, 2=water surface, 3=deep water, 4=rock):
- Bottom 6 rows: tileId 1 (solid seafloor)
- Rows 6–10 from bottom: tileId 3 (deep water filling most of zone)
- Row 11 from bottom: tileId 2 (water surface — player floats here)
- Scattered tileId 4 rocks on the seafloor
- Small sand islands: 3 platforms of tileId 1, each ~10 tiles wide, at varying heights

Zone data:
- name: "Shallows"
- biUnlockThreshold: 0.20
- 10 LitterObjects of type PLASTIC (oil spill patches) at seafloor level, visible=true
- WildlifeSlots: SeaTurtle (spawned=false), Starfish (spawned=false)
- RecycleStation at x=400 (on one of the sand islands), y = island surface

The oil spill patches should cluster in 2–3 groups rather than being evenly spaced.
```

---

### 1.1.2 — Zone 2: Transition trigger from Beach

```
In our C++/SDL2 game, implement a zone transition trigger on the right edge of the Beach zone.

1. Add to Zone struct in zone.h:
   struct ZoneTransition {
       int triggerX, triggerY, triggerW, triggerH;  // pixel-space trigger rect
       std::string targetZoneId;
       float requiredBI;  // minimum BI to allow transition
   };
   std::vector<ZoneTransition> transitions;

2. In createBeachZone(), add a transition:
   triggerX = 3800, triggerY = 0, triggerW = 40, triggerH = 720
   targetZoneId = "shallows", requiredBI = 0.20

3. In Game::update(), after player position update:
   - For each transition in currentZone.transitions:
     - Check if player bounding box overlaps trigger rect
     - If biodiversityIndex >= transition.requiredBI: call switchZone(transition.targetZoneId)
     - Else: block player movement past triggerX, show "BI too low" flash message

4. Implement switchZone(const std::string& id):
   - Load the target zone
   - Reset player.x to 100 (left side of new zone), keep player.y
   - Reset camera.x = 0

Show only the new/changed code sections.
```

---

### 1.2.1 — Zone 3: Meadow tilemap

```
Create `src/zones/meadow.h` with Zone createMeadowZone() for our C++/SDL2 game.

Tilemap spec (240×45 tiles):
- Bottom 8 rows: tileId 1 (flat grassy ground) — this is the flattest zone
- Occasional tileId 4 (rocks) scattered on the surface, height 2–3 tiles
- 2 shallow depressions (2 tiles deep, 8 tiles wide) filled with tileId 2 (puddle)
- No platforms — terrain stays mostly flat

Zone data:
- name: "Meadow"
- biUnlockThreshold: 0.40
- 10 LitterObjects of type CHEMICAL at ground level, visible=true
- 2 invisible CHEMICAL drums (visible=false, revealed by Baba Sela quest logic — leave as false for now)
- WildlifeSlots: Hare (spawned=false), BeeColony (spawned=false)
- RecycleStation at x=300, y=surface level (ground - 32px)

Add a transition from Shallows → Meadow on Shallows' right edge (same pattern as Beach → Shallows).
```

---

### 1.2.2 — Wildflower bloom visual trigger at BI 40%

```
In our C++/SDL2 game, when the Biodiversity Index reaches 40%, the Meadow zone should show
wildflower decorations on the ground tiles.

1. Add to TileMap: std::vector<int> decorationLayer — same size as tile array,
   decorationId 0 = none, 1 = wildflower, 2 = tall grass, 3 = mushroom

2. Add to Zone struct: bool wildflowersBlooomed = false

3. In Meadow zone createMeadowZone(), populate decorationLayer with:
   - decorationId 1 (wildflower) on ~30% of surface-level air tiles (tileId 0 above tileId 1)
   - Mark all as inactive initially by storing them in a separate pendingDecorations list

4. In Game::update(), if currentZone.name == "Meadow" && !currentZone.wildflowersBloomed
   && biodiversityIndex >= 0.40:
   - Set wildflowersBloomed = true
   - Copy pendingDecorations into decorationLayer
   - Call playSFX("bee_ambience") (placeholder)

5. In drawTileMap(), after drawing solid tiles, iterate decorationLayer:
   - decorationId 1: draw a small green cross / flower shape (4 pixels, SDL primitives)
     at tile center, offset 4px above tile top
   - decorationId 2: draw a thin vertical green rect (2×8px)
```

---

### 1.3.1 — Zone 4: Forest tilemap

```
Create `src/zones/forest.h` with Zone createForestZone() for our C++/SDL2 game.

Tilemap spec (240×45 tiles):
- Varied elevation: ground height oscillates between row 8 and row 16 from bottom
- tileId 1 fills from ground row to bottom
- tileId 4 (rocks) clustered in groups of 3–5
- 3 elevated platforms (tileId 1, 5 tiles wide, floating 6–8 tiles above ground)
- No water tiles

Zone data:
- name: "Forest"
- biUnlockThreshold: 0.60 (reduced to 0.50 if sela_helped == true — handle in switchZone logic)
- 10 LitterObjects of type MACHINERY at ground level, visible=true
- 3 LitterObjects of type MACHINERY, visible=false (Baba Sela reveals these)
- WildlifeSlots: Deer (spawned=false), Woodpecker (spawned=false)
- RecycleStation at x=500, y=surface level

At BI 60%, birch leaf decorations (decorationId 4) should appear on all air tiles in
the top 10 rows above any solid tile — same deferred mechanism as wildflowers in Meadow.
```

---

### 1.3.2 — Birch leaf render at BI 60%

```
In our C++/SDL2 game, at BI 60% in the Forest zone, birch leaves should render fully.

Use the same decoration system as wildflowers (decorationLayer, pendingDecorations).

For decorationId 4 (birch leaves):
- Draw a cluster of 5–8 small filled circles (SDL has no circle primitive — approximate with
  overlapping SDL_FillRect calls in a diamond pattern, radius ~3px) in muted green (80,120,60)
- Position: top-left corner of the tile, slight random offset stored as sub-tile offsets
  (store xOff, yOff per decoration entry, range -4 to +4, seeded by tile index)

Additionally:
- At BI 60%, spawn Deer entity: a wandering AnimalEntity that moves left/right slowly,
  reverses direction when it hits a wall or zone edge
- Spawn Woodpecker entity: stays near a tree column (any solid tile with air above),
  plays a 2-frame pecking animation on a 1-second interval

Both animals should use placeholder coloured rects until real sprites are available.
```

---

### 1.4.1 — Zone 5: Hill tilemap

```
Create `src/zones/hill.h` with Zone createHillZone() for our C++/SDL2 game.

Tilemap spec (240×45 tiles):
- Ascending terrain: ground rises from row 12 at x=0 to row 6 at x=240 (left to right climb)
- tileId 1 fills below the ground line
- tileId 4 (rocks) densely placed — this is the rockiest zone
- Narrow ledge paths: 3 horizontal platforms, each 8 tiles wide, staggered to require jumping

Special object: "broadcast tower" — not a tile, but a LitterObject of type MACHINERY
positioned at x=3500, y=groundLevel-64 (tall, takes 2 player heights)

Zone data:
- name: "Hill"
- biUnlockThreshold: 0.80
- 8 LitterObjects of type MACHINERY (tower debris) visible=true
- 1 LitterObject of type MACHINERY (the tower itself) visible=true, requiresCrane=true
- WildlifeSlots: Eagle (spawned=false, uses circle-overhead animation), LichenMoss (spawned=false)
- No recycle station — player must return to previous zone to recycle

Add transition Hill → [end zone / credits trigger] on the right edge.
```

---

### 1.4.2 — Eagle circle overhead animation at BI 80%

```
In our C++/SDL2 game, when BI reaches 80% and the player is in the Hill zone, an eagle
should appear and circle overhead.

Implement EagleEntity (extends AnimalEntity):
1. State machine: HIDDEN → CIRCLING → PERCHED
2. CIRCLING state:
   - Eagle moves in a large elliptical path above the zone
   - centerX = camera.x + 640 (always above player's screen center)
   - centerY = 120 (near top of screen)
   - ellipse radius: 300px horizontal, 80px vertical
   - angle advances at 0.8 radians/second
   - eagle.x = centerX + cos(angle) * 300
   - eagle.y = centerY + sin(angle) * 80
3. Render: if sprite null, draw a dark brown/grey V-shape (two thin rects at ~30° angle)
   representing wings in soaring pose
4. After 30 seconds of circling, transition to PERCHED:
   - Pick a tile with tileId 4 (rock) surface near x=2000
   - Eagle glides to that position over 2 seconds (lerp)
   - In PERCHED state: play slow 2-frame idle animation (head turns)

Trigger: in Game::update(), if biodiversityIndex >= 0.80 && currentZone.name == "Hill"
&& !eagleSpawned: create eagle entity, set state=CIRCLING, eagleSpawned=true.
```

---

### 1.5.1 — BI threshold check for zone gates

```
In our C++/SDL2 game, zone transitions must check the Biodiversity Index.

Update the zone transition system in Game::update():

1. When player overlaps a ZoneTransition trigger rect:
   a. If biodiversityIndex >= transition.requiredBI → allow: call switchZone()
   b. If biodiversityIndex < transition.requiredBI → block:
      - Halt player velocity: player.velX = 0, push player back by 4px
      - Set a flash message: "Need {requiredBI*100}% BI to enter" with a 2-second timer
      - Play sfx "zonegate_locked" (placeholder)

2. Add a visual "gate" drawn at each transition trigger position:
   - A vertical line of alternating coloured dashes (dim when locked, bright when BI met)
   - Use SDL_SetRenderDrawColor: locked = (180,60,60,200), unlocked = (60,180,80,200)
   - 8px wide, full zone height, drawn at transition.triggerX

3. Flash message renderer: small text box at bottom-center of screen, visible for 2 seconds,
   fades out over 0.5s (reduce alpha from 255 to 0)

Since SDL_ttf may not be linked yet, render the message as a pre-drawn texture or use
a simple dot-matrix font system (or skip text and use a colour-coded icon for now).
```

---

### 1.5.2 — Smooth camera transition between zones

```
In our C++/SDL2 game, when the player switches zones, the camera should transition smoothly
rather than cutting instantly.

Implement a CameraTransition system:

1. Add to Camera struct:
   bool transitioning
   float transitionProgress  // 0.0 to 1.0
   float transitionSpeed     // default 3.0 (completes in ~0.33 seconds)
   int fromX, toX            // camera X at start and end of transition

2. In switchZone():
   - Store fromX = camera.x
   - Load new zone, position player at new zone start
   - Set toX = 0 (new zone always starts at camera x=0)
   - Set transitioning = true, transitionProgress = 0.0

3. In Game::update(), if camera.transitioning:
   - transitionProgress += deltaTime * transitionSpeed
   - camera.x = lerp(fromX, toX, easeInOut(transitionProgress))
   - if transitionProgress >= 1.0: transitioning = false
   - During transition, disable player input (player.inputEnabled = false)

4. easeInOut(t): return t < 0.5 ? 2*t*t : -1+(4-2*t)*t

5. Add a brief white flash (alpha 180, decays over 0.3s) at transition midpoint to
   signal zone change to the player.
```

---

## PHASE 2 — Economy & Tools

---

### 2.1.1 — Add LitterType enum and credit values

```
In `src/zone.h` for our C++/SDL2 game, update LitterType and credit award logic.

1. Ensure LitterType enum is:
   enum class LitterType { PLASTIC, ORGANIC, CHEMICAL, MACHINERY };

2. Add a free function: int creditsForLitter(LitterType type)
   - PLASTIC → 1
   - ORGANIC → 2 (upgradeable to 3 via Tomek's favour)
   - CHEMICAL → 2
   - MACHINERY → 3

3. In the pickup handler (wherever collected litter is processed):
   - Call creditsForLitter(litter.type) and add to gameState.credits
   - Play sfx "pickup"
   - If type == ORGANIC and tomek_helped == true: award 3 instead of 2

4. Update the BI calculation: biodiversityIndex should increase by a fixed amount
   per litter collected — total litter across all zones = 63 items, so each pickup
   contributes (1.0 / 63) to the BI.
   Update biodiversityIndex after every pickup and clamp to [0.0, 1.0].

Show only the changed/new code.
```

---

### 2.2.1 — Implement carry limit and Burden slow

```
In our C++/SDL2 game, the player can carry at most 10 litter items before suffering
a speed penalty.

1. Player struct already has: int carryCount, bool hasBurden

2. In the pickup handler, after adding to carryCount:
   - if carryCount > 10: hasBurden = true
   - else: hasBurden = false

3. In updatePlayer(), apply speed modifier:
   - Base speed: 180.0f px/s
   - Burden speed: 90.0f px/s (50% reduction)
   - float speed = player.hasBurden ? 90.0f : 180.0f

4. Prevent picking up more than 20 items total (hard cap):
   - if carryCount >= 20: block pickup, show flash message "Drop off first"

5. In HUD, add carry indicator:
   - A row of 10 small squares (8×8px each) at bottom-right of screen
   - Filled squares = items carried (green up to 10, red if >10)
   - If hasBurden: draw a small weight/slowness icon (down-arrow shape) next to counter
```

---

### 2.2.2 — HUD carry count indicator

```
In `src/hud.cpp` of our C++/SDL2 game, add a carry count indicator to the HUD.

Position: bottom-right corner, 20px from edge. Layout: 10 small squares in a row.

void renderCarryIndicator(SDL_Renderer* r, int carryCount, bool hasBurden):
1. Draw 10 squares, each 10×10px with 3px gap
   - squares 0 to min(carryCount,10)-1: filled green (50,200,80)
   - squares carryCount to 9: outline only, dim (60,60,80)
   - if carryCount > 10: squares 0–9 all filled red (200,60,60), plus "+N" overflow text
2. Below the squares: if hasBurden, draw a downward arrow (3 SDL_FillRect calls:
   a vertical bar 3×8px + two diagonal lines approximated as rectangles at 45°) in orange
3. Above the squares: a small label (coloured dot pattern spelling "BAG" or just a
   backpack icon made of 2 rectangles)

Add this call to the main renderHUD() function.
```

---

### 2.3.1 — Place and implement recycle stations

```
In our C++/SDL2 game, each zone (except Hill) has a recycle station. Implement it.

1. RecycleStation is already stored in Zone as int recycleStationX, recycleStationY.
   Render it in renderZone() as a distinct visual:
   - A 32×48px structure: bottom half = grey rectangle (bin shape), top = darker lid
   - A small coloured stripe on the side: green for ORGANIC, blue for PLASTIC/CHEMICAL,
     yellow for MACHINERY
   - An [E] prompt rendered above it when player is within 64px

2. In Game::update(), check player proximity to recycle station:
   - If distance(player, recycleStation) < 64px AND player presses [E]:
     - Award credits: sum creditsForLitter(type) for each carried item
     - Play sfx "recycle"
     - Clear player inventory (carryCount = 0, hasBurden = false)
     - Trigger credit counter animation (see 2.3.2)

3. The [E] proximity prompt: a small pulsing rectangle with "E" pattern (dot-matrix or
   coloured rect) that scales between 0.9× and 1.1× size on a 0.5s sine cycle.
```

---

### 2.3.2 — Credit counter animation

```
In our C++/SDL2 game, when the player earns Creator Credits at a recycle station,
show an animated credit counter tick-up.

Implement a CreditPopup system:

struct CreditPopup {
    int amount;      // credits just earned
    float x, y;      // screen position (above recycle station)
    float lifetime;  // starts at 1.5, counts down
    float alpha;     // 255 → 0 over lifetime
};

1. Add std::vector<CreditPopup> activePopups to Game struct

2. When credits are awarded: push a CreditPopup { amount, stationScreenX, stationScreenY-40, 1.5f, 255 }

3. In Game::update(), for each popup:
   - lifetime -= deltaTime
   - y -= 30.0f * deltaTime  (floats upward)
   - alpha = (lifetime / 1.5f) * 255
   - Remove if lifetime <= 0

4. In renderHUD(), for each popup:
   - Draw "+ N CR" as dot-matrix text (or coloured rect placeholder) at (x, y)
   - Apply alpha via SDL_SetTextureAlphaMod if using a texture, or SDL_SetRenderDrawColor alpha

5. Also animate the total credit counter in the HUD:
   - When credits change, store displayedCredits (float) and lerp it toward actualCredits
     at rate 20 credits/second so the counter visibly ticks up
```

---

### 2.4.1 — Define Tool structs

```
Create `src/tools.h` in our C++/SDL2 game with the Tool Library system.

enum class ToolType { HAND_TROWEL, PRUNING_SAW, DRONE_CAMERA, BIOFILTER_KIT, CRANE_ARM };

struct Tool {
    ToolType type;
    std::string name;
    int borrowCost;       // in Creator Credits; 0 = free
    bool borrowed;        // currently held by player
    LitterType unlocks;   // which LitterType this tool enables cleanup of
};

Define all 5 tools as constants:
- HAND_TROWEL:   cost=0,  unlocks=PLASTIC (basic — always available without tool)
- PRUNING_SAW:   cost=3,  unlocks=ORGANIC (forest debris)
- DRONE_CAMERA:  cost=5,  unlocks=nothing specific — enables wildlife photography
- BIOFILTER_KIT: cost=8,  unlocks=CHEMICAL
- CRANE_ARM:     cost=12, unlocks=MACHINERY

Add to Game struct:
- Tool currentTool  (default = HAND_TROWEL)
- std::array<bool, 5> toolBorrowed  (all false initially)
- int toolLibraryX, toolLibraryY  (world position of the Tool Library station)

Tool Library station: place it in the Beach zone at x=100, y=surface-48.
```

---

### 2.4.2 — Tool Library UI

```
In our C++/SDL2 game, implement the Tool Library interaction UI.

Trigger: player presses [E] within 64px of the Tool Library station → toolMenuOpen = true.
Pause game update while toolMenuOpen.

Render the Tool Library menu (renderToolMenu in hud.cpp):
1. Semi-transparent overlay: SDL_SetRenderDrawColor(80,80,160,180), full-screen rect
2. Title area: row of coloured dots spelling "TOOL LIBRARY" at top-center
3. 5 rows, one per tool. Each row shows:
   - Tool name (dot-matrix or placeholder coloured label)
   - Cost in credits
   - Status: [BORROWED] if currently held, [BORROW X CR] if affordable, [NEED X CR] if not
   - Currently selected row highlighted with a brighter background rect
4. Navigation: [W]/[S] or arrow keys move selection, [E]/[Enter] confirms borrow/return,
   [Escape] closes menu

Borrow logic:
- If tool.borrowed: return it → credits NOT refunded (by design), toolBorrowed[i] = false
- If !tool.borrowed && credits >= borrowCost: deduct credits, set toolBorrowed[i] = true,
  currentTool = that tool
- Only one tool can be borrowed at a time (except HAND_TROWEL which is always "equipped")

Also: block litter pickup if litter.type requires a tool the player doesn't have.
Show "Need [TOOL NAME]" flash message if player tries to pick up locked litter.
```

---

### 2.4.3 — Tool return on zone exit

```
In our C++/SDL2 game, tools are automatically returned when the player exits a zone.

In switchZone():
1. If currentTool != HAND_TROWEL:
   - Set currentTool = HAND_TROWEL
   - Set toolBorrowed[currentToolIndex] = false
   - Show flash message: "[TOOL] returned"
   - Play sfx "recycle" (reuse for now)

Also handle manual return with [R] key:
In Game::update(), if key [R] pressed && currentTool != HAND_TROWEL:
   - Same return sequence as above
   - Player does NOT get credits refunded

Update the HUD to show the currently equipped tool:
- Bottom-left corner: small icon (coloured rect) + tool name abbreviation
- TROWEL=grey, SAW=brown, DRONE=cyan, BIOFILTER=green, CRANE=orange
```

---

## PHASE 3 — NPCs & Dialogue

---

### 3.1.1 — Define dialogue format

```
In our C++/SDL2 game, design and implement a dialogue data format.

Create `src/dialogue.h`. Use a simple plain-text format parsed at runtime (no JSON library needed).

Format example for a dialogue file (assets/dialogue/marta.txt):
---
[MARTA]
Hello, stranger. This beach used to be so clean.
[CONDITION:!marta_helped]
I've been trying to mend those nets over there, but my hands aren't what they were.
Can you pick up 5 pieces of plastic on this beach? Then I can show you something.
[/CONDITION]
[CONDITION:marta_helped]
You did it! The nets are mended. Take this Bio-filter kit — you'll need it.
[/CONDITION]
---

Parse rules:
- [NAME] sets the current speaker for following lines
- [CONDITION:flag] / [/CONDITION] — only show lines inside if flag is true in GameFlags
- Lines not in a tag are always shown
- Blank lines are ignored

Implement:
struct DialogueLine { std::string speaker; std::string text; };
struct Dialogue { std::vector<DialogueLine> lines; };
Dialogue loadDialogue(const char* path, const GameFlags& flags);

GameFlags struct: bool marta_helped, tomek_helped, sela_helped (all false by default).
Add GameFlags to the Game struct.
```

---

### 3.1.2 — Write DialogueManager

```
In `src/dialogue.h/.cpp` of our C++/SDL2 game, implement the DialogueManager.

struct DialogueManager {
    Dialogue current;
    int lineIndex;
    bool active;
    float displayTimer;   // time current line has been shown (for auto-advance option)
};

Functions:
void startDialogue(DialogueManager& dm, const Dialogue& d)
   - Sets dm.current = d, lineIndex = 0, active = true

bool advanceDialogue(DialogueManager& dm)
   - lineIndex++
   - if lineIndex >= lines.size(): active = false, return false (dialogue ended)
   - return true

void renderDialogue(SDL_Renderer* r, const DialogueManager& dm)
   - Only renders if dm.active
   - Bottom of screen: a 900×120px semi-transparent box (10,10,30,210 RGBA)
   - 2px border in accent colour (96,96,255)
   - Speaker name: top-left of box, in brighter colour (180,180,255)
   - Dialogue text: below speaker name, word-wrapped at 80 chars
   - Since SDL_ttf may not be available: render text as a placeholder grey rectangle
     (height 12px, width proportional to text.length()*7), speaker as a brighter rect
   - Bottom-right of box: "[SPACE] continue" indicator (small pulsing dot)

In Game::update(): if dialogueManager.active && SPACE pressed: advanceDialogue()
In renderHUD(): call renderDialogue() — it self-guards if not active.
```

---

### 3.1.3 — Render dialogue box in terminal style

```
In our C++/SDL2 game, the dialogue box should match the portfolio's terminal aesthetic
(dark background, monospace green/purple text, CRT-style).

Since we cannot use SDL_ttf, implement a bitmap font renderer using a hardcoded 5×7 pixel
font for the ASCII characters 32–126.

Create `src/font.h` with:
1. A 5×7 bitmap for each ASCII character stored as uint8_t bitmaps[95][7]
   (generate or hardcode a simple pixel font — start with just A–Z, 0–9, space, common punctuation)
2. void drawChar(SDL_Renderer* r, char c, int x, int y, SDL_Color color, int scale)
   - Looks up bitmap[c - 32], draws each set bit as a (scale × scale) filled rect
3. void drawText(SDL_Renderer* r, const char* text, int x, int y, SDL_Color color, int scale)
   - Iterates chars, calls drawChar, advances x by (5+1)*scale per character
4. int textWidth(const char* text, int scale) — returns total pixel width

Update renderDialogue() to use drawText() instead of placeholder rects:
- Speaker name: color=(180,180,255), scale=2
- Dialogue body: color=(160,200,160), scale=1
- Word wrap: break at 80 chars (or 640px width), advance y by 10px per line

Provide the full bitmap data for at minimum: A–Z uppercase, 0–9, space, . , ! ? ' - [ ]
```

---

### 3.2.1 — NPC Marta implementation

```
In our C++/SDL2 game, implement NPC Marta in the Beach zone.

1. In createBeachZone(), add an NPCEntity:
   - npcId = "marta"
   - x = 320, y = surface level - 32
   - width=16, height=32
   - favourComplete = false

2. Marta's placeholder sprite: a 16×32 coloured rect, yellow with a darker head section.

3. Proximity interaction: if player within 48px of Marta AND [E] pressed:
   - Load marta.txt dialogue filtered by current gameFlags
   - Start dialogue via dialogueManager.startDialogue()

4. Favour quest logic (check in Game::update()):
   - Track int plasticPickedUpInBeach (increment on PLASTIC pickup in Beach zone)
   - When plasticPickedUpInBeach >= 5 && !gameFlags.marta_helped:
     - gameFlags.marta_helped = true
     - toolBorrowed[BIOFILTER_KIT] = true (Marta loans it — no credit cost)
     - currentTool = BIOFILTER_KIT
     - Show flash message "Marta gave you the Bio-filter kit!"
     - On next interaction with Marta, dialogue loads the marta_helped=true branch

5. Create assets/dialogue/marta.txt with the dialogue content from Phase 3.1.1 format.
```

---

### 3.3.1 — NPC Tomek implementation

```
In our C++/SDL2 game, implement NPC Tomek in the Meadow zone.

1. In createMeadowZone(), add an NPCEntity:
   - npcId = "tomek"
   - x = 600, y = surface level - 32
   - favourComplete = false

2. Favour quest logic:
   - Track int chemicalDrumsWithBiofilter (increment when player cleans a CHEMICAL litter
     item while BIOFILTER_KIT is the currentTool)
   - When chemicalDrumsWithBiofilter >= 3 && !gameFlags.tomek_helped:
     - gameFlags.tomek_helped = true
     - organicCreditBonus = true (ORGANIC litter now awards 3cr instead of 2)
     - Show flash message "Tomek taught you better composting! Organic = +3cr"

3. Create assets/dialogue/tomek.txt:
   ---
   [TOMEK]
   These chemical drums have been leaching into the soil for years.
   [CONDITION:!tomek_helped]
   If you have a Bio-filter kit, you can neutralise them. Clean three for me?
   [/CONDITION]
   [CONDITION:tomek_helped]
   You've done it. The soil can breathe again. I've taught you the composting upgrade.
   [/CONDITION]
   ---

4. Tomek placeholder sprite: yellow-green tinted 16×32 rect with a darker hat section.
```

---

### 3.4.1 — NPC Baba Sela implementation

```
In our C++/SDL2 game, implement NPC Baba Sela in the Forest zone.

1. In createForestZone(), add an NPCEntity:
   - npcId = "sela"
   - x = 800, y = surface level - 32
   - favourComplete = false

2. Favour: Sela reveals 3 hidden MACHINERY litter objects in the Forest zone.
   - When player talks to Sela (any interaction):
     - gameFlags.sela_helped = true
     - Find all LitterObjects in Forest zone where visible=false → set visible=true
     - Flash message: "Baba Sela revealed hidden pollution sources!"
     - Forest zone's biUnlockThreshold is reduced: in switchZone(), when loading Forest,
       check if sela_helped → override threshold from 0.60 to 0.50

3. Create assets/dialogue/sela.txt:
   ---
   [BABA SELA]
   I have lived in this forest longer than you have been alive.
   I know where the poison hides. Let me show you.
   [CONDITION:sela_helped]
   You can feel it now, can't you? The forest remembers everything.
   [/CONDITION]
   ---

4. Sela placeholder sprite: purple-tinted 16×32 rect, shorter (16×24px) to suggest age.
   Idle animation: very slow (0.5 fps) 2-frame bob.
```

---

### 3.5.1 — NPC favour tracking and state persistence

```
In our C++/SDL2 game, all NPC regard flags must persist in the GameState and affect
dialogue + rewards consistently.

1. GameFlags struct (already defined) holds: marta_helped, tomek_helped, sela_helped.
   Add to Game struct: GameFlags flags;

2. Implement saveGameState(const std::string& path) and loadGameState(const std::string& path):
   - Format: plain text, one key=value per line
   - Keys: marta_helped, tomek_helped, sela_helped, biodiversityIndex, credits,
     currentZone, playerX, playerY
   - Use std::ofstream / std::ifstream
   - For Emscripten: use FS.syncfs() after write — add ASYNCIFY or use Emscripten's
     persistent storage via -s FORCE_FILESYSTEM=1 and EM_ASM({ FS.syncfs(false, ...) })

3. Auto-save triggers: after every recycle station visit, after every NPC favour completes,
   on zone transition.

4. On Game::init(): attempt loadGameState("savefile.txt") — if file not found, start fresh.

5. Verify: after sela_helped=true is set, calling loadDialogue("sela.txt", flags) must
   return only the sela_helped=true branch. Write a unit test function
   void testDialogueFlags() that asserts this and runs on debug builds only.
```

---

## PHASE 4 — Progression & World Transformation

---

### 4.1.1 — Colour shift / desaturation system

```
In our C++/SDL2 game, each zone starts desaturated and gains colour as BI rises.

Since SDL2 doesn't have a built-in greyscale shader, implement colour tinting via
SDL_SetTextureColorMod on tile textures (or per-draw colour scaling for primitive draws).

For primitive-only rendering (no textures yet):
1. Add to Zone struct: float colourSaturation (0.0 to 1.0)
2. Write SDL_Color desaturate(SDL_Color base, float saturation):
   - Grey value: grey = 0.299*r + 0.587*g + 0.114*b
   - Return { lerp(grey,r,sat), lerp(grey,g,sat), lerp(grey,b,sat), 255 }
3. In drawTileMap(), replace all SDL_SetRenderDrawColor calls with:
   SDL_Color tinted = desaturate(BASE_COLOUR_FOR_TILE[tileId], currentZone.colourSaturation)
   SDL_SetRenderDrawColor(r, tinted.r, tinted.g, tinted.b, 255)
4. Update colourSaturation in Game::update():
   - Target saturation = clamp(biodiversityIndex * 2.0f, 0.0f, 1.0f) for Beach
     (fully coloured at BI 50%)
   - Smooth lerp: zone.colourSaturation = lerp(zone.colourSaturation, target, deltaTime * 0.5f)
5. Apply same desaturation to entity/animal colours.
```

---

### 4.2.1 — BI milestone triggers (all 5)

```
In Game::update() of our C++/SDL2 game, add a milestone check system that fires once per threshold.

struct Milestone {
    float biThreshold;
    bool fired;
    std::string id;
};

Add to Game struct: std::array<Milestone, 5> milestones = {
    { 0.20f, false, "first_life" },
    { 0.40f, false, "meadow_awakens" },
    { 0.60f, false, "forest_stirs" },
    { 0.80f, false, "summit_cleared" },
    { 1.00f, false, "leave_no_trace" }
};

In Game::update(), after any BI change:
for (auto& m : milestones) {
    if (!m.fired && biodiversityIndex >= m.biThreshold) {
        m.fired = true;
        triggerMilestone(m.id);
    }
}

Implement triggerMilestone(const std::string& id):
- "first_life": spawnAnimal(Beach, "Crab"), spawnAnimal(Beach, "Seagull"), playSFX("seagull")
- "meadow_awakens": bloom wildflowers in Meadow zone (set pendingDecorations active),
  playMusic layer 2 (placeholder)
- "forest_stirs": bloom birch leaves in Forest zone, spawnAnimal(Forest, "Deer"),
  spawnAnimal(Forest, "Woodpecker"), playMusic layer 3
- "summit_cleared": spawnEagle(Hill), playMusic layer 4
- "leave_no_trace": checkWinCondition()

Each trigger also shows a 3-second full-screen text card (see 4.2.2).
```

---

### 4.2.2 — Milestone text cards

```
In our C++/SDL2 game, when a BI milestone fires, display a brief atmospheric text card.

Implement renderMilestoneCard(SDL_Renderer* r, const std::string& message, float progress):
- progress: 0.0–1.0 (lifetime / 3.0 seconds)
- Background: full-screen black rect with alpha = sin(progress * PI) * 180
  (fades in then out)
- Text: centered on screen using drawText() from font.h
  - message lines, colour (140,200,140), scale=2
- Sub-text: smaller line below, colour (100,140,100), scale=1

Milestone messages:
- "first_life":      "life returns\nto the shore"
- "meadow_awakens":  "the meadow\nremembers colour"
- "forest_stirs":    "something stirs\nin the birches"
- "summit_cleared":  "the eagle\ncircles again"
- "leave_no_trace":  "the island\nbreathes"

Add to Game struct: std::string activeMilestoneMessage; float milestoneCardTimer (0 = inactive).
In Game::update(): milestoneCardTimer -= deltaTime; if <= 0, clear message.
In Game::render(): if milestoneCardTimer > 0, call renderMilestoneCard().
During card display: DO NOT pause gameplay — let the player keep moving.
```

---

### 4.3.1 — Mindfulness moment [M]

```
In our C++/SDL2 game, pressing [M] triggers a 10-second mindfulness pause.

Implement in Game::update() and Game::render():

State: bool mindfulnessActive, float mindfulnessTimer (10.0f when active).

On [M] key pressed (not during dialogue or tool menu):
1. mindfulnessActive = true, mindfulnessTimer = 10.0f
2. Pause all entity updates (skip update loops while active)
3. player.stamina = 1.0f (instant restore)
4. playSFX("mindfulness_ambience") — a looping calm ambient track

During mindfulness (render):
1. Full-screen dark overlay: SDL_SetRenderDrawColor(0, 0, 0, 160), SDL_RenderFillRect full screen
2. World still visible underneath (overlay is semi-transparent)
3. Centred text: "breathe" in colour (140,180,140), scale=3 using drawText()
4. Below: a timer bar — thin horizontal rect, width decreasing from 400px to 0 over 10s
   colour (100,160,100)
5. Small hint: "[any key] to continue" at bottom-center, scale=1

On any key pressed while active: end mindfulness early (mindfulnessActive = false).
Auto-end after 10 seconds: mindfulnessTimer -= deltaTime; if <= 0: end.

Do NOT allow mindfulness during the ending cinematic.
```

---

### 4.4.1 — Wildlife Photo Atlas

```
In our C++/SDL2 game, implement the Wildlife Photo Atlas.

1. Atlas struct in `src/atlas.h`:
   struct AtlasEntry {
       std::string speciesName;
       std::string description;   // 1 line, ~60 chars
       bool photographed;
       SDL_Color accentColour;    // used for silhouette / revealed sprite tint
   };
   struct Atlas { std::vector<AtlasEntry> entries; };

2. Populate Atlas with all 12 species:
   Seagull, Crab, SeaTurtle, Starfish, Hare, BeeColony, Deer, Woodpecker, Eagle, LichenMoss,
   + 2 bonus species (e.g., Butterfly, Salamander — spawned randomly at BI >60%)

3. Photo action: while Drone Camera tool is borrowed, [F] key pressed:
   - Check if any AnimalEntity is within 200px of player
   - If yes: find matching AtlasEntry by speciesName, set photographed=true
     playSFX("atlas_unlock"), show flash "+Atlas: [species]"
   - If no: show flash "No wildlife nearby"

4. Atlas UI ([Tab] opens/closes):
   renderAtlas(SDL_Renderer* r, const Atlas& atlas):
   - Full-screen dark overlay
   - Title "FIELD ATLAS" top-center
   - Grid of entries, 4 columns: each cell 180×80px
     - If !photographed: grey silhouette (filled rect of accentColour at 30% brightness)
       + "???" name
     - If photographed: coloured rect + species name + 1-line description
   - Current count bottom: "X / 12 photographed"
   - [Tab] or [Escape] to close
```

---

## PHASE 5 — Win Condition & Ending

---

### 5.1.1 — Structure dismantling

```
In our C++/SDL2 game, the player must dismantle all placed structures to trigger the win condition.

"Structures" are objects placed by the player in the world — specifically Tool Library stations
and recycle station platforms. Tag all of these as dismantleable.

1. Add to a generic PlacedObject struct:
   bool isDismantleable = true
   bool dismantled = false
   float dismantleProgress = 0.0f  // 0.0 to 1.0, fills over 2 seconds of holding [E]

2. Track in Game struct:
   int structuresRemaining = 3  // 1 Tool Library + 2 recycle stations (Beach, Meadow)
   (Hill has no recycle station; Forest has 1)
   Adjust count based on your actual placed structures.

3. In Game::update(), for each PlacedObject within 64px of player, if [E] held:
   - dismantleProgress += deltaTime / 2.0f  (fills over 2 seconds)
   - If dismantleProgress >= 1.0f:
     - dismantled = true
     - credits += 3
     - structuresRemaining--
     - playSFX("recycle")
     - Show flash "+3cr — structure returned"

4. Render a progress arc/bar above the structure while [E] is held:
   - A small horizontal bar (48×6px) above the object
   - Fill from left: progress * 48px, colour (100,200,100)
   - Background rect in dim (40,40,60)
```

---

### 5.2.1 — Win condition check

```
In our C++/SDL2 game, after every BI update and after every structure dismantle,
check the win condition.

void Game::checkWinCondition():
- if biodiversityIndex >= 1.0f && structuresRemaining <= 0:
  - winTriggered = true
  - stopMusic()
  - beginEndingCinematic()

Add bool winTriggered = false to Game struct.

Guard against double-triggering: once winTriggered = true, skip all further
checkWinCondition() calls.

Also: add a HUD indicator showing progress toward win condition:
- Bottom of screen, small text: "BI: 100% ✓  Structures: N remaining"
- Only show when biodiversityIndex >= 0.9f (close to win) to avoid cluttering early game
- Colour: dim until both conditions met, then bright green
```

---

### 5.3.1 — Ending cinematic

```
In our C++/SDL2 game, implement the ending cinematic triggered after win condition.

struct EndingCinematic {
    int phase;         // 0–4
    float phaseTimer;
    bool complete;
};

Phase sequence:
0 — Fade to black over 3 seconds (alpha 0→255 overlay)
1 — Text card 1: "the island breathes" — 4 seconds
2 — Text card 2: "the raft departs" — 4 seconds
3 — Text card 3: "you left nothing behind\nexcept seeds" — 5 seconds
4 — Credits roll — scrolling list of names, ~15 seconds

During cinematic: disable all input except [Skip] (press [Escape] to jump to credits phase).

Implement renderEndingCinematic(SDL_Renderer* r, const EndingCinematic& ec):
- Phase 0: black overlay with increasing alpha
- Phases 1–3: full black + centered text via drawText() colour (160,220,160), scale=2
  sub-line in scale=1
- Phase 4: scrolling credits
  - Store credits as vector<string>
  - All text scrolls upward at 40px/second
  - scrollY starts at 720, ends when last line scrolls off top (scrollY < -totalHeight)
  - When phase 4 complete: exitGame() or show main menu placeholder

Play music "ending_cinematic" at phase 0 start (fade in over 2s).
```

---

## PHASE 6 — Audio Pass

---

### 6.1.1 — Music layering system

```
In our C++/SDL2 game, implement a layered music system where new instrument layers
are added as BI milestones are reached.

Since SDL2_mixer supports multiple channels, we can play multiple OGG loops simultaneously.

Update AudioManager:
1. Add: std::array<int, 6> musicChannels — each is a Mix_PlayChannel index
2. Add: std::array<Mix_Chunk*, 6> musicLayers — looping chunks (not Mix_Music, use chunks
   on dedicated channels for multi-layer support)
3. loadMusicLayer(int index, const char* path): loads as Mix_LoadWAV (looping chunk)
4. activateMusicLayer(int index): Mix_PlayChannel(musicChannels[index], musicLayers[index], -1)
   with a 2-second volume fade in using Mix_FadeInChannel
5. Layers to load on Game::init():
   - Layer 0: music/layer_ocean.ogg (always playing from start)
   - Layer 1: music/layer_zone2.ogg (add at BI 20%)
   - Layer 2: music/layer_zone3.ogg (add at BI 40%)
   - Layer 3: music/layer_zone4.ogg (add at BI 60%)
   - Layer 4: music/layer_zone5.ogg (add at BI 80%)
   - Layer 5: music/ending.ogg (play in ending cinematic, others stop)

For placeholder: all files are 1-second silence OGGs. The system should work silently
until real audio assets are provided.
```

---

### 6.2.1 — SFX manifest and integration

```
In our C++/SDL2 game, load and wire all ~40 required SFX to their trigger points.

Create `assets/audio/sfx/` with placeholder silence files for each:

Footsteps (called every 0.3s during walk, based on surface tile type):
- step_sand.ogg, step_water.ogg, step_grass.ogg, step_wood.ogg, step_stone.ogg

In updatePlayer(): track float stepTimer -= deltaTime; when <= 0:
  reset stepTimer = 0.3f, determine surface tile under player, playSFX("step_" + surface)

Interactions:
- sfx_pickup.ogg — on litter pickup
- sfx_recycle.ogg — on recycle station deposit
- sfx_compost.ogg — on ORGANIC litter pickup specifically
- sfx_zonegate_open.ogg — on first entry to a new zone
- sfx_zonegate_locked.ogg — on blocked zone entry attempt
- sfx_tool_borrow.ogg — on Tool Library borrow
- sfx_tool_return.ogg — on tool return
- sfx_dismantle.ogg — on structure dismantle complete
- sfx_atlas_unlock.ogg — on new photo taken

Animal calls (play when animal entity is within 400px of player, cooldown 8s):
- sfx_seagull.ogg, sfx_crab.ogg, sfx_turtle.ogg, sfx_hare.ogg,
  sfx_bee.ogg, sfx_deer.ogg, sfx_woodpecker.ogg, sfx_eagle.ogg

UI:
- sfx_dialogue_advance.ogg — on space in dialogue
- sfx_mindfulness.ogg — mindfulness moment start

Wire each playSFX() call to the corresponding game event listed above.
```

---

## PHASE 7 — Polish & Ship

---

### 7.1.1 — Diegetic HUD

```
In our C++/SDL2 game, replace the debug overlay HUD with diegetic (world-embedded) UI elements.

1. BI Meter — "Carved stone tablet" on the Beach zone:
   - A large stone-coloured rect (64×200px) at fixed world position x=80, y=400
   - Inside: a vertical fill bar representing biodiversityIndex (0–100%)
   - Fill colour: lerp from (100,80,60) at 0% to (60,180,80) at 100%
   - Small moss/leaf decoration drawn above bar at current fill level (simple rects)
   - Camera-anchored: always visible on screen (not world-space) at screen position 20, 260

2. Credit counter — "Shell pile":
   - At screen position bottom-left: a cluster of small circles (5px radius each)
   - One circle per credit, up to 20 visible; above 20: show "×N" text
   - Circles colour: warm sandy (200,180,120)
   - Animate: when credit count changes, a new circle "falls" from +30px above, lerping down

3. Carry count — "Bag icon":
   - Simple backpack shape: a 20×24 rect (bag body) + small 10×8 rect on top (flap)
   - Fill level shown as a darker inner rect proportional to carryCount/20
   - Screen position: bottom-right, 20px from edge
   - Turns red when hasBurden = true

Remove: the old debug text HUD (BI%, credit number, carry number as raw text) unless
in debug build mode (add #ifdef DEBUG guard).
```

---

### 7.2.1 — Full playthrough test checklist

```
Before shipping our C++/SDL2/Emscripten game, run through this checklist and fix each issue.

Write a debug console command system: pressing [~] toggles a debug panel showing:
- biodiversityIndex (editable with +/- keys for testing)
- currentZone name
- player position (x, y)
- credits
- structuresRemaining
- All gameFlags (marta_helped, tomek_helped, sela_helped)
- milestone.fired states
- FPS counter

Test cases to verify manually:
1. Walk from Beach to Hill by artificially setting BI to 1.0 — all 5 zone gates open
2. BI gate blocks entry when BI too low — push-back and flash message appear
3. Collect 20 items — Burden slow activates, carry indicator goes red
4. Borrow all 5 tools in sequence — only one active at a time
5. Complete Marta's favour — Bio-filter kit granted without credit cost
6. Complete Tomek's favour — organic litter awards 3cr
7. Talk to Baba Sela — 3 hidden Forest litter become visible
8. Reach BI 100% + dismantle all structures → ending cinematic plays
9. [M] mindfulness — stamina restores, pause lasts 10s or exits on keypress
10. Save and reload — all flags, BI, credits, position preserved

Fix all bugs encountered. Log each fix as a comment in the relevant source file.
```

---

### 7.3.1 — itch.io web build

```
Prepare the final Emscripten build for itch.io upload.

Update Makefile with a new target `release-web`:
em++ src/*.cpp \
  -o build/index.html \
  -s USE_SDL=2 \
  -s USE_SDL_IMAGE=2 \
  -s USE_SDL_MIXER=2 \
  -s SDL2_IMAGE_FORMATS='["png"]' \
  -s SDL2_MIXER_FORMATS='["ogg"]' \
  -s WASM=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=67108864 \
  -O3 \
  --closure 1 \
  --shell-file shell.html \
  --preload-file assets \
  -s ASYNCIFY=1 \
  -s FORCE_FILESYSTEM=1

Update shell.html for itch.io:
- Remove the portfolio "back" button
- Set canvas to 800×600 (itch.io default embed size)
- Add a fullscreen button (standard itch.io pattern)
- Remove terminal intro sequence from shell — game starts immediately
- Add: <meta name="itch:dimensions" content="800x600">

Test in: Chrome 120+, Firefox 120+, Safari 17+
Known issues to check: Safari requires user gesture before audio; add a "Click to Start"
overlay that enables AudioContext on click before the game loop begins.
```

---

### 7.4.1 — Steam PC build

```
Prepare a native PC build of our C++/SDL2 game for Steam.

1. Create a Makefile target `release-pc`:
   - Linux: g++ src/*.cpp -o zielony-plywak -lSDL2 -lSDL2_image -lSDL2_mixer -O2
   - Windows (cross-compile from Linux): use mingw:
     x86_64-w64-mingw32-g++ src/*.cpp -o zielony-plywak.exe [SDL2 mingw flags] -O2

2. Emscripten-specific guards: wrap all EM_ASM and emscripten.h calls with:
   #ifdef __EMSCRIPTEN__ ... #endif
   so native builds compile cleanly.

3. Asset path handling:
   - WASM: assets are preloaded by Emscripten into virtual FS — paths work as-is
   - Native: assets must be in the same directory as the executable
   - Use a helper: std::string assetPath(const char* relative) that prepends
     the executable directory on native builds, returns relative path on WASM

4. Save file location:
   - WASM: Emscripten persistent FS (IndexedDB)
   - Linux: ~/.local/share/zielony-plywak/save.txt
   - Windows: %APPDATA%\ZielonyPlywak\save.txt
   Use SDL_GetPrefPath("WolnikRadek", "ZielonyPlywak") — works cross-platform.

5. Steam integration (optional MVP): add a steam_stub.h with empty Steamworks calls
   (#define SteamAPI_Init() true etc.) so the build works without the actual SDK.
   Real Steamworks can be swapped in later.
```

---

*End of PROMPTS.md — 52 implementation prompts covering all 7 phases.*
