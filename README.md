# CS467_Emulator - 8080 Space Invaders Emulator

This project is a software emulator for the Intel 8080 microprocessor, specifically designed to run the classic 1978 arcade game Space Invaders in its original, unmodified form. Built as a CS 467 capstone project, this emulator provides hands-on experience with fundamental computer science concepts including CPU instruction cycles, memory management, interrupt handling, and low-level hardware emulation. Written in C and developed collaboratively using modern software engineering practices, the emulator faithfully recreates the original arcade hardware environment, allowing the authentic Space Invaders ROM to execute exactly as it did on the original machine.

## Repository Structure
```
8080-space-invaders-emulator/
├── .github/                   # GitHub workflows and templates
├── src/                       # Source code
│   ├── cpu/                   # Intel 8080 CPU emulation
│   │   ├── disassembler.h     # Disassembler header file
│   │   ├── disassembler.c     # Core disassembler functions
│   │   ├── disassembler_main.c# Standalone disassembler program
│   │   └── emulator_shell.c   # Main emulator with disassembler helper
│   ├── memory/                # Memory system and ROM loading
│   ├── graphics/              # Display and rendering
│   └── io/                    # Input/output and hardware ports
├── tests/                     # Testing framework and test cases
├── roms/                      # Space Invaders ROM files
├── docs/                      # Project documentation
├── scripts/                   # Build and utility scripts
├── build/                     # Compiled object files (created by make)
├── bin/                       # Executable files (created by make)
├── Makefile                   # Build system configuration
├── .gitignore                 # Git ignore rules
└── README.md                  # This file
```

### Current Status
This project is in early development. The basic directory structure has been established to organize code by functional areas:
- **`src/cpu/`**: Intel 8080 CPU emulation with modular disassembler
- **`src/memory/`**: Memory management and ROM loading
- **`src/graphics/`**: Graphics rendering and display output
- **`src/io/`**: Input/output handling and hardware port emulation
- **`tests/`**: Will contain unit tests and CPU diagnostic tests
- **`roms/`**: Directory for Space Invaders ROM files (to be acquired separately)
- **`docs/`**: Project documentation and setup guides
- **`scripts/`**: Build and environment setup scripts

### Getting Started

```bash
# Clone the repository
git clone [repository-url]
cd CS467_Emulator

# Set up WSL2 development environment (once implemented)
./scripts/setup-wsl2.sh
```

## Using Makefile
The Makefile provides a consistent build system for the project, supporting both a standalone disassembler tool and a complete emulator. The build system creates two directories automatically:

- `/build`: Contains compiled object files (.o files)
- `/bin`: Contains final executable files

### Modular Disassembler Architecture
The project uses a modular approach for the disassembler:

- `disassembler.c`: Core disassembly functions (no main function)
- `disassembler.h`: Header file with function declarations
- `disassembler_main.c`: Standalone disassembler program with main()
- `emulator_shell.c`: Main emulator that includes disassembler as helper

This allows the same disassembly code to be used both as a standalone ROM analysis tool and as a debugging helper within the emulator.

### Building with Makefile
```bash
# Build the emulator (default)
make

# Build standalone disassembler only
make disassemble

# Build both emulator and disassembler
make both

# Clean build artifacts
make clean

# Clean and rebuild everything
make clean && make both
```

### Available Commands
```
make              - Build emulator only (default)
make disassemble  - Build standalone disassembler only  
make both         - Build both emulator and disassembler
make test         - Test emulator with ROM file
make debug        - Debug build of emulator
make clean        - Clean up build files
make status       - Show project status and file listings
make help         - Show detailed help and usage information
```

### Testing
```bash
# Test the emulator (requires ROM file in roms/ directory)
make test

# Use standalone disassembler to analyze ROM
make disassemble
./bin/disassembler roms/space_invaders/invaders > disassembly.txt
```

### Adding New Files to Makefile
When adding new source files to the project:

1. Add filename to appropriate SOURCES variable
```makefile
CPU_SOURCES = $(CPU_DIR)/disassembler.c $(CPU_DIR)/emulator_shell.c $(CPU_DIR)/new_file.c
```
2. Add object file to appropriate OBJECTS variable
```makefile
NEW_OBJECTS = $(BUILD_DIR)/cpu/new_file.o
```
3. Add compilation rule for your file
```makefile
$(BUILD_DIR)/cpu/new_file.o: $(CPU_DIR)/new_file.c $(CPU_DIR)/new_file.h
    @mkdir -p $(BUILD_DIR)/cpu
    $(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
```
4. Rebuild the project
```makefile
make clean && make
```

### Expected File Structure
The build system expects these key files to exist:
- `src/cpu/disassembler.h` - Header file with function declarations
- `src/cpu/disassembler.c` - Core disassembler implementation (no main function)
- `src/cpu/disassembler_main.c` - Standalone disassembler with main() function
- `src/cpu/emulator_shell.c` - Main emulator code (includes disassembler.h)

### Development Workflow
1. Make changes to source files
2. Run `make clean && make` to rebuild
3. Test with `make test` (for emulator) or run `./bin/disassembler <rom_file>` (for disassembler)
4. Use `make status` to verify all files are present and accounted for