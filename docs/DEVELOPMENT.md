# Development Guide

This guide covers IDE setup, and in the future will also include debugging techniques, and advanced configuration for developing the Intel 8080 emulator.

## Table of Contents
- [VS Code Setup](#vs-code-setup)
- [Graphics](#graphics)
- [Debugging](#debugging) (future)
- [Code Style](#code-style) (future)
- [Common Development Tasks](#common-development-tasks) (future)

## VS Code Setup

If you plan to use Visual Studio Code for development or code browsing, follow these steps to configure the C/C++ extension properly.

### Initial Setup

1. Install the C/C++ extension for VS Code:
   ```
   ext install ms-vscode.cpptools
   ```

2. Create a `.vscode` directory in your project root:
   ```bash
   mkdir -p .vscode
   ```

### Configuration Files

#### For macOS with Homebrew

Create `.vscode/c_cpp_properties.json`:

```json
{
  "configurations": [
    {
      "name": "Mac",
      "includePath": [
        "${workspaceFolder}/**",
        "/opt/homebrew/include/SDL2",
        "/opt/homebrew/include"
      ],
      "defines": [],
      "macFrameworkPath": [
        "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks"
      ],
      "compilerPath": "/opt/homebrew/bin/gcc-13",
      "cStandard": "c17",
      "cppStandard": "c++17",
      "intelliSenseMode": "macos-gcc-arm64"
    }
  ],
  "version": 4
}
```

> **Note**: Adjust `compilerPath` based on your gcc version. Check with `ls /opt/homebrew/bin/gcc-*`

#### For Linux/WSL2

Create `.vscode/c_cpp_properties.json`:

```json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": [
        "${workspaceFolder}/**",
        "/usr/include/SDL2"
      ],
      "defines": [],
      "compilerPath": "/usr/bin/gcc",
      "cStandard": "c17",
      "cppStandard": "c++17",
      "intelliSenseMode": "linux-gcc-x64"
    }
  ],
  "version": 4
}
```

#### All Platforms

Create `.vscode/settings.json`:

```json
{
  "C_Cpp.default.compilerPath": "/usr/bin/gcc",
  "files.associations": {
    "*.h": "c",
    "*.c": "c"
  },
  "C_Cpp.errorSquiggles": "enabled"
}
```


## Graphics

The graphics directory contains source files used in emulating and rendering the graphical elements

## Background and Specifications

*   Space Invaders has a resolution of 224 pixels (width) by 256 pixels (height).
*   Graphics are drawn in memory (VRAM) from 0x2400 to 0x3FFF, which the video controller reads and renders onto the hardware display[^1][^2].
*   Each bit represents a pixel, and is rendered left to right, and then rotated 90 degrees counterclockwise (vertical retrace) to give the user a 224 (wide) by 256 (high) screen[^3].

## Technologies Used

The Simple DirectMedia Layer (SDL) library is used to handle screen emulation. SDL will emulate the game image window.

[^1]: https://web.archive.org/web/20240718053956/http://emulator101.com/
[^2]: https://web.archive.org/web/20240119084322/https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html#screen-geometry
[^3]: https://www.walkofmind.com/programming/side/hardware.htm#:~:text=The%20physical%20video%20screen%2C%20however,a%20%22standard%22%20screen%20layout.

## Setup

1.  Run `sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev` to install the SDL development libraries.
2.  Compile the graphics program using `gcc graphics.c -o graphics -lSDL2` (SDL2 library link).
3.  Run using `.\graphics` (or other executable name).
4.  Quit by closing the graphics window (or Ctrl+C in terminal).

Run `sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev libsdl2-net-dev` to install the SDL development libraries.

## Resources
*   [SDL2 Wiki](https://wiki.libsdl.org/SDL2/FrontPage)