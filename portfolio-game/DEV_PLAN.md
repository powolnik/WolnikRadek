# Zielony Pływak — Atomized Development Plan
> C++ · SDL2 · Emscripten · WebAssembly  
> Target: ~3–4 hr cosy side-scroller, PC (Steam) + Web (itch.io demo)  
> Team: Solo / Duo  
> Updated: March 2026

---

## ✅ DONE — Current State

- [x] Project structure & portfolio integration
- [x] C++/SDL2/Emscripten build pipeline working
- [x] WASM deployed to GitHub Pages via `portfolio-game/build/`
- [x] Eco meter (Biodiversity Index) core logic
- [x] Player walk + jump + swim movement
- [x] 3 zones with procedural terrain (Beach, Forest, Wetlands)
- [x] 45 trash items — pickup mechanic
- [x] 12 animal types spawning at BI milestones
- [x] HUD: eco bar, trash counter, mini-map
- [x] Particle effects system

---

## PHASE 0 — Foundation & Cleanup
> ~1–2 weeks | Goal: solid base before expanding

### 0.1 — Replace procedural terrain with hand-crafted zones
- [x] Define tile map data structure (layer arrays or JSON)
- [x] Draw Beach zone tilemap (15 screens wide target)
- [x] Replace runtime terrain gen with tilemap renderer in SDL2
- [x] Verify camera scroll across full zone width

### 0.2 — Art pipeline setup
- [x] Agree on sprite sheet format (16×16 tiles, PNG with transparency)
- [x] Write SDL2 sprite sheet loader (`loadTilesheet`, `drawTile`)
- [x] Write animation system: frame index + frame timer per entity
- [x] Test with placeholder 16×16 player sprite (4-frame idle, 6-frame walk, 3-frame pickup)

### 0.3 — Code architecture refactor
- [x] Split `main.cpp` into modules: `game.cpp`, `player.cpp`, `zone.cpp`, `hud.cpp`, `entity.cpp`
- [x] Define `Zone` struct: tilemap, litter list, wildlife slots, BI unlock threshold
- [x] Define `Entity` base class: position, sprite, update(), draw()

### 0.4 — SDL2_mixer integration (audio scaffolding)
- [x] Add `SDL2_mixer` to Makefile / Emscripten flags
- [x] Write `AudioManager`: load, play, stop, set volume
- [x] Add placeholder audio slots: `sfx_pickup`, `sfx_recycle`, `music_base`

---

## PHASE 1 — All 5 Zones Blocked Out
> ~3–4 weeks | Goal: walkable island, all zones accessible via BI gates

### 1.1 — Zone 2: Shallows (unlock at BI 20%)
- [x] Tilemap: tidal pool terrain, water surface tiles
- [x] Litter set: 8–12 oil spill patch objects
- [x] Wildlife slots: Sea turtle, Starfish
- [x] Zone transition trigger (right edge of Beach → Shallows)

### 1.2 — Zone 3: Meadow (unlock at BI 40%)
- [x] Tilemap: coastal grass, flat with scattered rocks
- [x] Litter set: 8–12 chemical drum objects
- [x] Wildlife slots: Hare, Bee colony
- [ ] Wildflower bloom visual trigger at BI 40%

### 1.3 — Zone 4: Forest (unlock at BI 60%)
- [x] Tilemap: birch grove, varied elevation
- [x] Litter set: 8–12 abandoned machinery objects
- [x] Wildlife slots: Deer, Woodpecker
- [ ] Birch leaves render fully at BI 60%

### 1.4 — Zone 5: Hill (unlock at BI 80%)
- [x] Tilemap: rocky summit, narrow paths
- [x] Litter set: old broadcast tower + surrounding debris
- [x] Wildlife slots: Eagle, Lichen moss
- [ ] Eagle circle overhead animation at BI 80%

### 1.5 — Zone gate system
- [x] BI threshold check before allowing zone entry
- [x] Visual indicator on zone border (locked/unlocked state)
- [x] Smooth camera transition between zones

---

## PHASE 2 — Economy & Tools
> ~2 weeks | Goal: Creator Credits loop fully functional

### 2.1 — Litter classification
- [x] Add `LitterType` enum: `PLASTIC`, `ORGANIC`, `CHEMICAL`, `MACHINERY`
- [x] Assign credits per type: plastic +1, organic compost +2, structure dismantle +3
- [x] Update pickup logic to award credits by type

### 2.2 — Inventory & Burden system
- [x] Implement carry limit: max 10 items
- [x] Apply Burden slow (speed penalty) when carrying >10
- [x] HUD indicator for current carry count

### 2.3 — Drop-off / recycle points
- [x] Place recycle station per zone (Beach, Meadow, Forest)
- [x] Interaction: dump carried items → award credits → clear inventory
- [x] Visual feedback: credit counter ticks up

### 2.4 — Tool Library
- [x] Define tool structs: Hand trowel (free), Pruning saw (3cr), Drone camera (5cr), Bio-filter kit (8cr), Crane arm (12cr)
- [x] Tool borrow UI: press [E] at Tool Library station → menu opens
- [x] Lock tool-specific litter types until correct tool is borrowed
- [x] Return tool on zone exit or manual [R] return action

---

## PHASE 3 — NPCs & Dialogue
> ~2–3 weeks | Goal: 3 NPCs with favour quests working end-to-end

### 3.1 — Dialogue engine
- [x] Define dialogue format (plain text or JSON): lines, speaker, condition flags
- [x] Write `DialogueManager`: load file, advance on [Space], show speaker name
- [x] Render dialogue box in terminal/diegetic style matching portfolio aesthetic
- [x] Support `[condition]` flags to gate dialogue branches

### 3.2 — NPC: Marta (Beach hut)
- [x] Sprite: unique 16×16 pixel art (contract or placeholder)
- [x] Favour: mend torn fishing nets → requires 5 plastic pickups in Beach zone
- [x] Reward: Bio-filter kit loaned (skips 8cr cost)
- [x] Regard flag: `marta_helped = true`

### 3.3 — NPC: Tomek (Meadow camp)
- [x] Sprite: unique 16×16
- [x] Favour: seed rare coastal grass → requires Bio-filter kit used on 3 chemical drums
- [x] Reward: composting upgrade (organic waste gives +3cr instead of +2cr)
- [x] Regard flag: `tomek_helped = true`

### 3.4 — NPC: Baba Sela (Birch grove)
- [x] Sprite: unique 16×16
- [x] Favour: identify hidden pollution → reveals 3 invisible litter objects in Forest
- [x] Reward: Forest zone unlocks at BI 50% instead of 60%
- [x] Regard flag: `sela_helped = true`

### 3.5 — NPC favour tracking
- [x] Persist regard flags in game state struct
- [x] Gate rewards behind `regard == true` checks
- [x] NPC dialogue changes after favour complete

---

## PHASE 4 — Progression & World Transformation
> ~2 weeks | Goal: island visually heals as BI rises

### 4.1 — Colour shift system
- [x] Each zone starts desaturated (apply SDL greyscale tint to tiles)
- [x] On cleanup events: smooth hue-saturation tween to full colour
- [x] Per-zone colour state saved in `Zone` struct

### 4.2 — BI milestone triggers
- [x] BI 20%: crabs + seagulls appear on Beach, ambient crab sounds
- [x] BI 40%: wildflower tiles replace barren grass in Meadow, bee ambience
- [x] BI 60%: birch leaf tiles fully rendered, deer track decals appear
- [x] BI 80%: eagle circle animation plays over Hill zone, music layer 4 adds
- [x] BI 100%: win condition check → trigger ending sequence

### 4.3 — Mindfulness moment [M]
- [x] Press [M] anywhere → 10-second ambient pause
- [x] Black overlay fades in, ambient sound plays (waves/wind/birdsong)
- [x] Energy/stamina fully restores after pause
- [x] Press any key to exit early

### 4.4 — Wildlife Photo Atlas
- [x] `Atlas` struct: one entry per species, `photographed` bool
- [x] Drone camera tool enables [F] photo action near wildlife
- [x] Atlas UI screen accessible from HUD ([Tab])
- [x] Atlas entry: species name, silhouette → full sprite when photographed

---

## PHASE 5 — Win Condition & Ending
> ~1 week

### 5.1 — Structure dismantling
- [x] Tag all player-placed objects (Tool Library stations are "owned")
- [x] Add dismantle interaction: hold [E] 2 seconds → remove structure, +3cr
- [x] Track `structures_remaining` count in game state

### 5.2 — Win condition check
- [x] After every BI update: check `bi >= 100 && structures_remaining == 0`
- [x] If true: trigger ending cinematic

### 5.3 — Ending cinematic
- [x] Fade to black scripted sequence
- [x] Text cards: 3–4 lines ("the island breathes", "the raft departs")
- [x] Ambient soundscape plays over cards (wired — awaiting audio asset)
- [x] Credits roll (scrolling text renderer)
- [x] Return to main menu or desktop

---

## PHASE 6 — Audio Pass
> ~1 week | Requires audio assets from composer

### 6.1 — Music
- [x] Ocean base layer (code wired — awaiting audio asset)
- [x] Zone 2 instrument layer adds at BI 20% (code wired — awaiting audio asset)
- [x] Zone 3 instrument layer adds at BI 40% (code wired — awaiting audio asset)
- [x] Zone 4 instrument layer adds at BI 60% (code wired — awaiting audio asset)
- [x] Zone 5 instrument layer adds at BI 80% (code wired — awaiting audio asset)
- [x] Ending cinematic track (code wired — awaiting audio asset)

### 6.2 — SFX (~40 effects)
- [x] Footsteps: sand, water, grass, wood, stone (code wired — awaiting audio assets)
- [x] Pickup chime, recycle crunch, compost squelch (code wired — awaiting audio assets)
- [x] Animal calls: seagull, crab, turtle, hare, bee, deer, woodpecker, eagle (code wired — awaiting audio assets)
- [x] UI sounds: dialogue advance, atlas unlock, zone gate open (code wired — awaiting audio assets)
- [x] Mindfulness moment ambience loop (code wired — awaiting audio asset)

---

## PHASE 7 — Polish & Ship
> ~4 weeks

### 7.1 — Diegetic HUD
- [ ] Replace debug HUD with carved stone BI meter on Beach
- [ ] Credit counter shown as shell/token pile near player
- [ ] Carry count shown as bag icon only

### 7.2 — Bug fixing & playtesting
- [ ] Full playthrough test (3–4 hr target)
- [ ] Edge cases: BI gate re-lock, inventory overflow, tool return on death
- [ ] Performance: maintain 60fps in WASM at 1280×720

### 7.3 — itch.io web demo build
- [ ] Final Emscripten compile with `-O3` + `--closure 1`
- [ ] Update `shell.html` for itch.io embed (800×600 iframe)
- [ ] Test in Chrome, Firefox, Safari

### 7.4 — Steam PC build
- [ ] Install SDL2 natively, compile native Linux/Windows binary
- [ ] Integrate Steamworks SDK (optional for MVP)
- [ ] Prepare Steam page: description, screenshots, capsule art

---

## Budget Estimate

| Item | Cost |
|---|---|
| Pixel artist (contract) | €1 500 – €3 000 |
| Composer (contract) | €500 – €1 000 |
| SDL2 / tools | €0 (open source) |
| Steam Direct fee | €100 |
| Marketing (itch.io + social) | €200 – €500 |
| **Total** | **~€2 300 – €4 600** |

---

## Success Metrics (Post-Launch)

- 500 downloads on itch.io within 30 days
- 200 paid copies on Steam at launch (~€10 break-even)
- Average session ≥ 25 min
- Completion rate ≥ 60%
- At least 1 cosy-game Let's Play / review

---

> **Rule:** Expand scope only after vertical slice (Phase 0–2) is playable and tested.
