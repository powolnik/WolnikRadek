# ─── Zielony Pływak – Build Configuration ───────────────────────────────────
# Requires: Emscripten SDK (emsdk) activated in your environment
#
# Quick start:
#   source /path/to/emsdk/emsdk_env.sh
#   make
#   # then open index.html via a local server (e.g. python3 -m http.server)

SRC        = src/main.cpp src/spritesheet.cpp src/audio.cpp
OUT_DIR    = build
OUT        = $(OUT_DIR)/game.js
SHELL_HTML = shell.html
FINAL      = $(OUT_DIR)/index.html

CC         = emcc
CFLAGS     = -std=c++17 -O2 -Wall -Wextra -Isrc
LDFLAGS    = -s USE_SDL=2 \
             -s USE_SDL_IMAGE=2 \
             -s SDL2_IMAGE_FORMATS='["png"]' \
             -s ALLOW_MEMORY_GROWTH=1 \
             -s INITIAL_MEMORY=33554432 \
             -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
             -s NO_EXIT_RUNTIME=1 \
             --shell-file $(SHELL_HTML)

.PHONY: all clean debug serve

all: $(FINAL)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(FINAL): $(SRC) $(SHELL_HTML) | $(OUT_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SRC) -o $(FINAL)
	@echo ""
	@echo "✓ Build complete! Files in $(OUT_DIR)/"
	@echo "  Serve with: cd $(OUT_DIR) && python3 -m http.server 8080"

debug: CFLAGS = -std=c++17 -O0 -g -Wall -Wextra -DDEBUG
debug: $(FINAL)
	@echo "✓ Debug build complete"

clean:
	rm -rf $(OUT_DIR)

serve: $(FINAL)
	cd $(OUT_DIR) && python3 -m http.server 8080
