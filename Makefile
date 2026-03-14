# ─────────────────────────────────────────────────────────────────────────────
#  Portfolio Game – Emscripten / WebAssembly Build
#
#  Requirements:
#    emscripten (emcc / em++ must be on PATH or sourced via emsdk_env.sh)
#
#  Usage:
#    make            – debug build (fast iteration)
#    make release    – optimised build
#    make serve      – serve build/ on http://localhost:8080
#    make clean      – remove build artefacts
# ─────────────────────────────────────────────────────────────────────────────

CXX       := em++
BUILD_DIR := build
SRC       := src/main.cpp
TARGET    := $(BUILD_DIR)/index.html
SHELL_FILE:= shell.html

# ── Emscripten flags shared by all builds ────────────────────────────────────
EM_FLAGS := \
  -s USE_SDL=2            \
  -s WASM=1               \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s MAX_WEBGL_VERSION=2   \
  -s MIN_WEBGL_VERSION=2   \
  -s ENVIRONMENT=web       \
  --shell-file $(SHELL_FILE)

# ── Debug (default) ──────────────────────────────────────────────────────────
CXXFLAGS_DEBUG := -std=c++17 -g -O0 \
  -s ASSERTIONS=1          \
  -s SAFE_HEAP=1

# ── Release ──────────────────────────────────────────────────────────────────
CXXFLAGS_RELEASE := -std=c++17 -O3 \
  -s ASSERTIONS=0           \
  --closure 1

# ── Default target: debug ─────────────────────────────────────────────────────
.PHONY: all debug release serve clean

all: debug

debug: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_DEBUG) $(SRC) $(EM_FLAGS) -o $(TARGET)
	@echo ""
	@echo "  ✓  Debug build ready → $(BUILD_DIR)/"

release: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS_RELEASE) $(SRC) $(EM_FLAGS) -o $(TARGET)
	@echo ""
	@echo "  ✓  Release build ready → $(BUILD_DIR)/"

$(BUILD_DIR):
	mkdir -p $@

# ── Local dev server (requires Python 3) ─────────────────────────────────────
serve:
	@echo "Serving on http://localhost:8080"
	cd $(BUILD_DIR) && python3 -m http.server 8080

clean:
	rm -rf $(BUILD_DIR)
