# CS467_Emulator - 8080 Space Invaders Emulator

This project is a software emulator for the Intel 8080 microprocessor, specifically designed to run the classic 1978 arcade game Space Invaders in its original, unmodified form. Built as a CS 467 capstone project, this emulator provides hands-on experience with fundamental computer science concepts including CPU instruction cycles, memory management, interrupt handling, and low-level hardware emulation. Written in C and developed collaboratively using modern software engineering practices, the emulator faithfully recreates the original arcade hardware environment, allowing the authentic Space Invaders ROM to execute exactly as it did on the original machine.

## Repository Structure
```
8080-space-invaders-emulator/
├── .github/                   # GitHub workflows and templates
├── src/                       # Source code
│   ├── cpu/                   # Intel 8080 CPU emulation
│   ├── memory/                # Memory system and ROM loading
│   ├── graphics/              # Display and rendering
│   └── io/                    # Input/output and hardware ports
├── tests/                     # Testing framework and test cases
├── roms/                      # Space Invaders ROM files
├── docs/                      # Project documentation
├── scripts/                   # Build and utility scripts
├── Makefile                   # Build system configuration (once implemented)
├── .gitignore                # Git ignore rules
└── README.md                 # This file
```
### Current Status

This project is in early development. The basic directory structure has been established to organize code by functional areas:

- **`src/`**: Source code organized into modules for CPU emulation, memory management, graphics rendering, and I/O handling
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

# Build the project (once source files are implemented)
make clean && make all
```