#include <stdio.h>
#include <stdint.h>
#include <err.h>
#include <disassembler.h>

/*
 * Intel 8080 Disassembler - Standalone Command Line Tool
 * 
 * Reads an Intel 8080 ROM file and disassembles it into human-readable assembly code.
 * Each instruction is printed with its memory address and assembly mnemonic.
 * 
 * This program provides a standalone disassembler utility that can be used to analyze
 * ROM files independently of the emulator. It uses the same disassembly engine as the
 * emulator for consistency.
 *
 * Usage: ./disassembler <rom_file>
 * 
 * Parameters:
 *   argc - Number of command line arguments (expects 2: program name + ROM file)
 *   argv - Array of command line arguments
 *          argv[0] = program name
 *          argv[1] = path to ROM file to disassemble
 *
 * Output: Disassembled assembly code written to stdout in format:
 *         <address> <instruction>
 *         Example: 0000 NOP
 *                  0001 LXI    B,#$1234
 *
 * Return Values:
 *   0 - Success: ROM file was successfully disassembled
 *   1 - Error: Invalid arguments, file not found, read error, or file size mismatch
 *
 * Example Usage:
 *   ./disassembler roms/space_invaders/invaders
 *   ./disassembler roms/invaders > output.txt
 *
 * Note: Space Invaders ROM should be 8192 bytes. Other ROM sizes are supported
 *       but may generate warnings if instructions extend beyond file boundaries.
 */

int main(int argc, char *argv[]) {
  // Check if ROM file was provided as argument
  if (argc != 2) {
      fprintf(stderr, "Usage: %s <rom_file>\n", argv[0]);
      fprintf(stderr, "Example: %s roms/space_invaders/invaders\n", argv[0]);
      return 1;
  }

  FILE *fp = fopen(argv[1], "rb"); // Note: "rb" for binary files
  if (fp == NULL) {
    err(1, "Unable to open ROM file: %s\n", argv[1]);
  } else {
    printf("ROM file opened successfully: %s\n", argv[1]);
  }
  
  // Find file size. Alien Invaders should be 8192 bytes.
  fseek(fp, 0L, SEEK_END);
  long int file_size = ftell(fp);
  // Set pointer back to beginning of file.
  fseek(fp, 0L, SEEK_SET);

  // Create buffer and read file into buffer.
  unsigned char buffer[file_size];
  size_t bytes_read = fread(buffer, sizeof(char), sizeof(buffer), fp);
  fclose(fp);

  if (bytes_read != (size_t)file_size) { // Cast file_size to size_t to match bytes_read type
    err(1, "Failed to read complete ROM file.\n");
  }

  int pc = 0;

  while(pc < file_size) {
    pc = pc + disassembled8080Op(buffer, pc);    
  }

  if (pc > file_size) {
    printf("Warning: Instruction goes beyond file size boundary.\n");
  }

  return 0;
}