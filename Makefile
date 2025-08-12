# Intel 8080 Emulator Makefile
# CS 467 - Summer 2025

# Compiler and flags
# -Wall and -Wextra: show comprehensive warnings to help catch bugs early, like using uninitialized variables, unused parameters, etc.
# -g: include debug symbols for use with gdb debugger to set breakpoints and examine variables
# -O2: optimizes for performance (makes code run faster)
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 $(shell sdl2-config --cflags) -I/opt/homebrew/include
SDL_LIBS = $(shell sdl2-config --libs) -lSDL2_mixer

# Directories
BUILD_DIR = build
# directory to hold compiled object files (.o files)
BIN_DIR = bin
# directory to hold final executables
SRC_DIR = src
CPU_DIR = $(SRC_DIR)/cpu
MEMORY_DIR = $(SRC_DIR)/memory
GRAPHICS_DIR = $(SRC_DIR)/graphics
IO_DIR = $(SRC_DIR)/io
ROMS_DIR = roms

# Current source files
# CPU_SOURCES = $(CPU_DIR)/disassembler.c $(CPU_DIR)/emulator_shell.c
CPU_SOURCES = $(CPU_DIR)/emulator_shell.c
# Add more/uncomment files here as we create them:
# CPU_SOURCES += $(CPU_DIR)/cpu8080.c
# MEMORY_SOURCES = $(MEMORY_DIR)/memory.c
GRAPHICS_SOURCES = $(GRAPHICS_DIR)/graphics.c
GRAPHICS_OBJECTS = $(BUILD_DIR)/graphics/graphics.o
IO_SOURCES = $(IO_DIR)/input.c $(IO_DIR)/sound.c

# All sources (TO EXPAND: uncomment the += lines below as we add new modules)
ALL_SOURCES = $(CPU_SOURCES)
# ALL_SOURCES += $(MEMORY_SOURCES)
ALL_SOURCES += $(GRAPHICS_SOURCES)
ALL_SOURCES += $(IO_SOURCES)

# Object files 
# NOTE: Using explicit object file definitions to avoid path construction issues
# As we add more source files, we'll add their corresponding object files here
DISASM_OBJECTS = $(BUILD_DIR)/cpu/disassembler.o
DISASM_MAIN_OBJECTS = $(BUILD_DIR)/cpu/disassembler_main.o
EMULATOR_OBJECTS = $(BUILD_DIR)/cpu/emulator_shell.o
# Future object files to add:
# CPU_OBJECTS += $(BUILD_DIR)/cpu/cpu8080.o
# MEMORY_OBJECTS = $(BUILD_DIR)/memory/memory.o
GRAPHICS_OBJECTS = $(BUILD_DIR)/graphics/graphics.o
IO_OBJECTS = $(BUILD_DIR)/io/input.o $(BUILD_DIR)/io/sound.o

# All objects - expand this as we add new modules
ALL_OBJECTS = $(DISASM_OBJECTS) $(DISASM_MAIN_OBJECTS)
ALL_OBJECTS += $(EMULATOR_OBJECTS)
ALL_OBJECTS += $(GRAPHICS_OBJECTS)
ALL_OBJECTS += $(IO_OBJECTS)

# Targets
DISASM_TARGET = $(BIN_DIR)/disassembler
EMULATOR_TARGET = $(BIN_DIR)/emulator
# TO ADD FULL EMULATOR: uncomment when we have cpu + memory + graphics + io
# EMULATOR_TARGET = $(BIN_DIR)/space_invaders_emulator

# Include directories for header files  
# TO USE HEADER FILES: uncomment when we start creating .h files
# This tells compiler where to find our header files when we #include them
INCLUDES = -I$(CPU_DIR) -I$(MEMORY_DIR) -I$(GRAPHICS_DIR) -I$(IO_DIR)

# =============================================================================
# BUILD RULES
# =============================================================================

# Default target - what happens when you just type "make"
# Builds only the emulator (with disassembler as helper function)
all: $(EMULATOR_TARGET)

# Build standalone disassembler - accessed via "make disassemble"
disassemble: $(DISASM_TARGET)

# Build both targets
both: $(DISASM_TARGET) $(EMULATOR_TARGET)

# Build standalone disassembler (disassembler_main + disassembler)
$(DISASM_TARGET): $(DISASM_OBJECTS) $(DISASM_MAIN_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(DISASM_OBJECTS) $(DISASM_MAIN_OBJECTS)
	@echo "✓ Built standalone $(DISASM_TARGET) successfully!"

# Build emulator (emulator_shell + disassembler as helper)
$(EMULATOR_TARGET): $(EMULATOR_OBJECTS) $(DISASM_OBJECTS) $(GRAPHICS_OBJECTS) $(IO_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(SDL_LIBS)
	@echo "✓ Built $(EMULATOR_TARGET) successfully!"

# Compile disassembler core (no main function)
$(BUILD_DIR)/cpu/disassembler.o: $(CPU_DIR)/disassembler.c $(CPU_DIR)/disassembler.h
	@mkdir -p $(BUILD_DIR)/cpu
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile disassembler main (standalone program)
$(BUILD_DIR)/cpu/disassembler_main.o: $(CPU_DIR)/disassembler_main.c $(CPU_DIR)/disassembler.h
	@mkdir -p $(BUILD_DIR)/cpu
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile emulator shell (includes disassembler.h for helper function)
$(BUILD_DIR)/cpu/emulator_shell.o: $(CPU_DIR)/emulator_shell.c $(CPU_DIR)/disassembler.h
	@mkdir -p $(BUILD_DIR)/cpu
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Add more compilation rules as we create files:
# $(BUILD_DIR)/cpu/cpu8080.o: $(CPU_DIR)/cpu8080.c
# 	@mkdir -p $(BUILD_DIR)/cpu
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/memory/memory.o: $(MEMORY_DIR)/memory.c
# 	@mkdir -p $(BUILD_DIR)/memory
# 	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/graphics/graphics.o: $(GRAPHICS_DIR)/graphics.c
	@mkdir -p $(BUILD_DIR)/graphics
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/io/input.o: $(IO_DIR)/input.c
	@mkdir -p $(BUILD_DIR)/io
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ 

$(BUILD_DIR)/io/sound.o: $(IO_DIR)/sound.c
	@mkdir -p $(BUILD_DIR)/io
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Future emulator target (uncomment when ready)
# Note: When we add graphics/audio, we may also need -lSDL2_mixer for sound
# $(EMULATOR_TARGET): $(ALL_OBJECTS)
# 	@mkdir -p $(BIN_DIR)
# 	$(CC) $(CFLAGS) -o $@ $(ALL_OBJECTS) -lSDL2
# 	@echo "✓ Built $(EMULATOR_TARGET) successfully!"

# =============================================================================
# UTILITY TARGETS  
# =============================================================================

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: clean all

# Test emulator
test: $(EMULATOR_TARGET)
	@if [ -f "$(ROMS_DIR)/space_invaders/invaders" ]; then \
		echo "Running emulator with ROM file..."; \
		./$(EMULATOR_TARGET) $(ROMS_DIR)/space_invaders/invaders; \
	else \
		echo "Error: ROM file not found at $(ROMS_DIR)/space_invaders/invaders"; \
	fi

# Clean up
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f output.txt
	@echo "✓ Cleaned up"

# Show project status
status:
	@echo "Sources: $(ALL_SOURCES)"
	@echo "Objects: $(ALL_OBJECTS)"
	@echo "Disassembler Target: $(DISASM_TARGET)"
	@echo "Emulator Target: $(EMULATOR_TARGET)"
	@echo ""
	@echo "Files that exist:"
	@find $(SRC_DIR) -name "*.c" 2>/dev/null || echo "No .c files found"
	@find $(SRC_DIR) -name "*.h" 2>/dev/null || echo "No .h files found"

# Help
help:
	@echo "Commands:"
	@echo "  make              - Build emulator only (default)"
	@echo "  make disassemble  - Build standalone disassembler only"
	@echo "  make both         - Build both emulator and disassembler"
	@echo "  make test-disasm  - Test standalone disassembler with ROM"
	@echo "  make test-emulator- Test emulator with ROM"
	@echo "  make test         - Test both disassembler and emulator"
	@echo "  make debug        - Debug build of emulator"
	@echo "  make clean        - Clean up build files"
	@echo "  make status       - Show project status"
	@echo ""
	@echo "File structure expected:"
	@echo "  $(CPU_DIR)/disassembler.h     - Header file"
	@echo "  $(CPU_DIR)/disassembler.c     - Core disassembler (no main)"
	@echo "  $(CPU_DIR)/disassembler_main.c- Standalone disassembler main"
	@echo "  $(CPU_DIR)/emulator_shell.c   - Emulator (includes disassembler.h)"

# Install dependencies
install-deps:
	sudo apt update
	sudo apt install -y libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev
	@echo "✓ Dependencies installed"

.PHONY: all debug test clean status help install-deps