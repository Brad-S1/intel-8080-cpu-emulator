# Intel 8080 Emulator Makefile
# CS 467 - Summer 2025

# Compiler and flags
# -Wall and -Wextra: show comprehensive warnings to help catch bugs early, like using uninitialized variables, unused parameters, etc.
# -g: include debug symbols for use with gdb debugger to set breakpoints and examine variables
# -O2: optimizes for performance (makes code run faster)
CC = gcc
CFLAGS = -Wall -Wextra -g -O2

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
CPU_SOURCES = $(CPU_DIR)/disassembler.c $(CPU_DIR)/emulator_shell.c
# Add more/uncomment files here as we create them:
# CPU_SOURCES += $(CPU_DIR)/cpu8080.c
# MEMORY_SOURCES = $(MEMORY_DIR)/memory.c
# GRAPHICS_SOURCES = $(GRAPHICS_DIR)/graphics.c
# IO_SOURCES = $(IO_DIR)/input.c

# All sources (TO EXPAND: uncomment the += lines below as we add new modules)
ALL_SOURCES = $(CPU_SOURCES)
# ALL_SOURCES += $(MEMORY_SOURCES)
# ALL_SOURCES += $(GRAPHICS_SOURCES)
# ALL_SOURCES += $(IO_SOURCES)

# Object files 
# NOTE: Using explicit object file definitions to avoid path construction issues
# As we add more source files, we'll add their corresponding object files here
DISASM_OBJECTS = $(BUILD_DIR)/cpu/disassembler.o
EMULATOR_OBJECTS = $(BUILD_DIR)/cpu/emulator_shell.o
# Future object files to add:
# CPU_OBJECTS += $(BUILD_DIR)/cpu/cpu8080.o
# MEMORY_OBJECTS = $(BUILD_DIR)/memory/memory.o
# GRAPHICS_OBJECTS = $(BUILD_DIR)/graphics/graphics.o
# IO_OBJECTS = $(BUILD_DIR)/io/input.o

# All objects - expand this as we add new modules
ALL_OBJECTS = $(DISASM_OBJECTS)
ALL_OBJECTS += $(EMULATOR_OBJECTS)
# ALL_OBJECTS += $(GRAPHICS_OBJECTS)
# ALL_OBJECTS += $(IO_OBJECTS)

# Targets
# DISASM_TARGET = $(BIN_DIR)/disassembler
EMULATOR_TARGET = $(BIN_DIR)/emulator
# TO ADD FULL EMULATOR: uncomment when we have cpu + memory + graphics + io
# EMULATOR_TARGET = $(BIN_DIR)/space_invaders_emulator

# Include directories for header files  
# TO USE HEADER FILES: uncomment when we start creating .h files
# This tells compiler where to find our header files when we #include them
# INCLUDES = -I$(CPU_DIR) -I$(MEMORY_DIR) -I$(GRAPHICS_DIR) -I$(IO_DIR)

# =============================================================================
# BUILD RULES
# =============================================================================

# Default target - what happens when you just type "make"

all: $(EMULATOR_TARGET)
# all: $(DISASM_TARGET) $(EMULATOR_TARGET)

# Build disassembler
$(DISASM_TARGET): $(DISASM_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(DISASM_OBJECTS)
	@echo "✓ Built $(DISASM_TARGET) successfully!"

# Build emulator:
$(EMULATOR_TARGET): $(EMULATOR_OBJECTS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(EMULATOR_OBJECTS)
	@echo "✓ Built $(EMULATOR_TARGET) successfully!"

# Compile disassembler files
$(BUILD_DIR)/cpu/disassembler.o: $(CPU_DIR)/disassembler.c
	@mkdir -p $(BUILD_DIR)/cpu
	$(CC) $(CFLAGS) -c $< -o $@

# Compile emulator shell:
$(BUILD_DIR)/cpu/emulator_shell.o: $(CPU_DIR)/emulator_shell.c $(CPU_DIR)/disassembler.c
	@mkdir -p $(BUILD_DIR)/cpu
	$(CC) $(CFLAGS) -c $< -o $@

# Add more compilation rules as we create files:
# $(BUILD_DIR)/cpu/cpu8080.o: $(CPU_DIR)/cpu8080.c
# 	@mkdir -p $(BUILD_DIR)/cpu
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/memory/memory.o: $(MEMORY_DIR)/memory.c
# 	@mkdir -p $(BUILD_DIR)/memory
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/graphics/graphics.o: $(GRAPHICS_DIR)/graphics.c
# 	@mkdir -p $(BUILD_DIR)/graphics
# 	$(CC) $(CFLAGS) -c $< -o $@

# $(BUILD_DIR)/io/input.o: $(IO_DIR)/input.c
# 	@mkdir -p $(BUILD_DIR)/io
# 	$(CC) $(CFLAGS) -c $< -o $@

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
debug: clean $(DISASM_TARGET)

# Test with ROM files
test: $(DISASM_TARGET)
	@if [ -f "$(ROMS_DIR)/space_invaders/invaders" ]; then \
		echo "Testing with ROM file..."; \
		./$(DISASM_TARGET) $(ROMS_DIR)/space_invaders/invaders > output.txt; \
		echo "✓ Output saved to output.txt"; \
	elif [ -f "$(ROMS_DIR)/invaders" ]; then \
		echo "Testing with ROM file..."; \
		./$(DISASM_TARGET) $(ROMS_DIR)/invaders > output.txt; \
		echo "✓ Output saved to output.txt"; \
	else \
		echo "Put ROM file in $(ROMS_DIR)/ to test"; \
		echo "Expected: $(ROMS_DIR)/invaders or $(ROMS_DIR)/space_invaders/invaders"; \
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
	@echo "Target: $(DISASM_TARGET)"
	@echo ""
	@echo "Files that exist:"
	@find $(SRC_DIR) -name "*.c" 2>/dev/null || echo "No .c files found"

# Help
help:
	@echo "Commands:"
	@echo "  make          - Build project"
	@echo "  make test     - Test with ROM"
	@echo "  make debug    - Debug build"
	@echo "  make clean    - Clean up"
	@echo "  make status   - Show status"
	@echo ""
	@echo "To add new files:"
	@echo "1. Add filename to appropriate SOURCES variable"
	@echo "2. Add object file to appropriate OBJECTS variable"
	@echo "3. Add compilation rule for your file"
	@echo "4. Run: make clean && make"

# Install dependencies
install-deps:
	sudo apt update
	sudo apt install -y libsdl2-dev build-essential
	@echo "✓ Dependencies installed"

.PHONY: all debug test clean status help install-deps