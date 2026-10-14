CXX ?= c++
PKG_CONFIG ?= pkg-config
BUILD_DIR ?= build
TARGET := $(BUILD_DIR)/simulation
SDL_PACKAGES := sdl2 SDL2_ttf
SOURCES := main.cpp renderer.cpp simulation_cpu.cpp simulation_cuda_stub.cpp
HEADERS := simulation.h renderer.h

CPPFLAGS += -DSIMULATION_CPU_ONLY=1
CXXFLAGS += -std=c++17 -O2

.PHONY: all build run clean

all: build

build: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	@command -v "$(CXX)" >/dev/null 2>&1 || { echo "Error: a C++ compiler is required (install build-essential)." >&2; exit 1; }
	@command -v "$(PKG_CONFIG)" >/dev/null 2>&1 || { echo "Error: pkg-config is required (install pkg-config)." >&2; exit 1; }
	@$(PKG_CONFIG) --exists $(SDL_PACKAGES) || { echo "Error: SDL2 development files are required (install libsdl2-dev and libsdl2-ttf-dev)." >&2; exit 1; }
	@mkdir -p "$(BUILD_DIR)"
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $$($(PKG_CONFIG) --cflags $(SDL_PACKAGES)) $(SOURCES) -o "$@" $$($(PKG_CONFIG) --libs $(SDL_PACKAGES))

run: build
	"./$(TARGET)"

clean:
	rm -rf "$(BUILD_DIR)"
