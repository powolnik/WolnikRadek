# Zielony Pływak (The Green Floater) 🌿

A cosy 2D side-scrolling cleanup game built in C++ with SDL2, compiled to WebAssembly via Emscripten.

## Game Features
- **3 Zones**: Beach, Forest, Wetlands — each with unique terrain and atmosphere
- **Trash Cleanup**: Pick up 45 items of litter (bottles, cans, bags, tires, barrels)
- **Eco Meter**: Watch the island heal as you clean — the sky gets greener, trees get lusher
- **Wildlife Restoration**: Animals (crabs, birds, deer, frogs, fish) appear as eco improves
- **Procedural World**: Terrain, decorations, and trash are generated each playthrough
- **HUD**: Eco meter, trash counter, mini-map, zone name popups
- **Solarpunk Aesthetic**: All procedurally drawn — no sprite assets needed

## Controls
| Key | Action |
|-----|--------|
| WASD / Arrow Keys | Move |
| Space | Jump |
| W/Up (in water) | Swim up |
| E | Pick up trash |
| ESC | Pause |
| Enter | Start / Confirm |

## Building

### Prerequisites
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

### Build Steps
```bash
# Activate emsdk
source /path/to/emsdk/emsdk_env.sh

# Build
make

# Serve locally
make serve
# → Open http://localhost:8080
```

### Debug Build
```bash
make debug
```

## Project Structure
```
portfolio-game/
├── src/
│   └── main.cpp          # Complete game source (~1100 lines)
├── shell.html            # Emscripten HTML shell template (portfolio-themed)
├── Makefile              # Build configuration
├── README.md
└── build/                # Output (after building)
    ├── index.html        # Playable game
    ├── game.js           # Emscripten JS glue
    └── game.wasm         # WebAssembly binary
```

## Integration with Portfolio
After building, copy `build/` contents to `portfolio-game/` in the main portfolio:
```bash
cp build/index.html ../WolnikRadek-enter/portfolio-game/index.html
cp build/game.js ../WolnikRadek-enter/portfolio-game/game.js
cp build/game.wasm ../WolnikRadek-enter/portfolio-game/game.wasm
```

## Based on GDD
Implements the "Small Scope" version of the Zielony Pływak Game Design Document.
