# ─────────────────────────────────────────────────────────────────────────────
# OverclockCTL — Makefile
# ─────────────────────────────────────────────────────────────────────────────

PLATFORM ?= linux
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2

TARGET    := overclockctl
SRC_DIR   := src
BUILD_DIR := build

GTK_CFLAGS  := $(shell pkg-config --cflags gtkmm-4.0 libadwaita-1 2>/dev/null)
GTK_LIBS    := $(shell pkg-config --libs   gtkmm-4.0 libadwaita-1 2>/dev/null)
VK_CFLAGS   := $(shell pkg-config --cflags vulkan 2>/dev/null)
VK_LIBS     := $(shell pkg-config --libs   vulkan 2>/dev/null)
ifeq ($(VK_LIBS),)
  VK_LIBS   := -lvulkan
endif

ifeq ($(PLATFORM),windows)
  CXX       := x86_64-w64-mingw32-g++
  TARGET    := overclockctl.exe
  PLAT_DEF  := -DOC_PLATFORM_WINDOWS
  PLAT_LIBS := -lPowrProf -lDXGI -lole32
  VK_LIBS   := -lvulkan-1
else
  PLAT_DEF  := -DOC_PLATFORM_LINUX
  PLAT_LIBS := -lpthread
endif

CXXFLAGS += $(GTK_CFLAGS) $(VK_CFLAGS) $(PLAT_DEF)
LDFLAGS   := $(GTK_LIBS) $(VK_LIBS) $(PLAT_LIBS)

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

SHADER_SRC := shaders/benchmark.comp
SHADER_SPV := shaders/benchmark.spv

.PHONY: all clean install shaders installer-windows installer-linux

all: shaders $(TARGET)

shaders: $(SHADER_SPV)

$(SHADER_SPV): $(SHADER_SRC)
	@echo "[GLSL] Compiling compute shader..."
	@if command -v glslc >/dev/null 2>&1; then \
	    glslc -fshader-stage=compute $< -o $@ && echo "  -> $@"; \
	elif command -v glslangValidator >/dev/null 2>&1; then \
	    glslangValidator -V $< -o $@ && echo "  -> $@"; \
	else \
	    echo "  WARNING: No GLSL compiler found (install shaderc or glslang)"; \
	fi

$(TARGET): $(OBJS)
	@echo "[LINK] $@"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build OK: ./$(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "[CXX]  $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET).exe $(SHADER_SPV)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)

# ── Installers ────────────────────────────────────────────────────────────────
# Linux .sh installer  (just make sure install.sh is executable)
installer-linux:
	chmod +x install.sh
	@echo "Linux installer ready: ./install.sh"

# Windows .exe installer  (requires NSIS)
installer-windows:
	@echo "[NSIS] Building Windows installer..."
	@if command -v makensis >/dev/null 2>&1; then \
	    makensis -V2 installer/windows/installer.nsi && \
	    echo "Windows installer: installer/windows/OverclockCTL-Setup.exe"; \
	else \
	    echo "makensis not found. Install: sudo pacman -S nsis"; \
	fi

installers: installer-linux installer-windows
