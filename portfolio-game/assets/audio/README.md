# Audio Assets

Placeholder directory for audio files from the composer.
All paths are wired in `src/audio.cpp` — drop files here and rebuild with `-DUSE_SDL_MIXER`.

## Music (OGG format, looping)

Each track is a **cumulative mix** — later tracks include all previous layers baked in,
because SDL2_mixer supports only one active music stream at a time.

| File | Trigger |
|---|---|
| `music_ocean_base.ogg` | Game start (always plays) |
| `music_layer_shallows.ogg` | BI hits 20% (includes base) |
| `music_layer_meadow.ogg` | BI hits 40% (includes base + shallows) |
| `music_layer_forest.ogg` | BI hits 60% (includes base + shallows + meadow) |
| `music_layer_hill.ogg` | BI hits 80% (all layers) |
| `music_ending.ogg` | Ending cinematic |

## SFX (WAV format)

### Actions
| File | Trigger |
|---|---|
| `sfx_pickup.wav` | Litter picked up |
| `sfx_recycle.wav` | Recycling at station |
| `sfx_compost_squelch.wav` | Organic composting |
| `sfx_dismantle.wav` | Structure fully dismantled |
| `sfx_win_chime.wav` | Win condition met |

### Footsteps
- `sfx_step_sand.wav`, `sfx_step_water.wav`, `sfx_step_grass.wav`
- `sfx_step_stone.wav`, `sfx_step_wood.wav`

### UI / Interaction
| File | Trigger |
|---|---|
| `sfx_zone_gate_open.wav` | Zone BI milestone (20/40/60/80%) |
| `sfx_dialogue_advance.wav` | [Space] advances NPC dialogue |
| `sfx_atlas_unlock.wav` | New species photographed |
| `sfx_tool_borrow.wav` | Tool borrowed from library |
| `sfx_tool_return.wav` | Tool returned (future) |
| `sfx_mindfulness_ambience.wav` | [M] mindfulness moment starts |

### Animal calls
- `sfx_animal_seagull.wav`, `sfx_animal_crab.wav`, `sfx_animal_turtle.wav`
- `sfx_animal_hare.wav`, `sfx_animal_bee.wav`, `sfx_animal_deer.wav`
- `sfx_animal_woodpecker.wav`, `sfx_animal_eagle.wav`
