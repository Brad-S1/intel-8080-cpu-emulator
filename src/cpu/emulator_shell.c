#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include <stdbool.h>
#include <time.h>
#include <SDL.h>

#include "graphics.h"
#include "input.h"
#include "machine_io.h"
#include "disassembler.h"
#include "sound.h"

// constants
#define MEMORY_SIZE 0x10000  // 64KB (8080 has 16-bit memory bus)

// structure for 8080 processor condition flags (status bits)
typedef struct ConditionCodes {
  uint8_t   z:1;    // zero
  uint8_t   s:1;    // sign
  uint8_t   p:1;    // parity
  uint8_t   cy:1;   // carry
  uint8_t   ac:1;   // auxillary carry
  uint8_t   pad:3;  // unused bits 
} ConditionCodes;

// structure for 8080 CPU register state
typedef struct State8080 {
  uint8_t   a;                    // accumulator
  uint8_t   b;                    // general purpose registers (b through l)
  uint8_t   c;
  uint8_t   d;
  uint8_t   e;
  uint8_t   h;
  uint8_t   l;
  uint16_t  sp;                   // stack pointer
  uint16_t  pc;                   // program counter
  uint8_t   *memory;              // pointer to memory
  struct    ConditionCodes  cc;   // flag register
  uint8_t   int_enable;           // interrupt enable 
} State8080;


/*
 * Helper function that prints complete CPU state for debugging purposes.
 * Displays Registers, stack pointer, program counter, and condition flags.
 */
void print_state_code(State8080 *state) {
  // Used variables to help keep printf statement short and readable. 
  uint16_t sp = state->sp;
  uint16_t pc = state->pc;
  // Prints stack pointers and program counter.
  printf("SP: %04x, PC: %04x --- ", sp, pc);
  
  uint8_t a = state->a;
  uint8_t b = state->b;
  uint8_t c = state->c;
  uint8_t d = state->d;
  uint8_t e = state->e;
  uint8_t h = state->h;
  uint8_t l = state->l;
  uint8_t int_enable = state->int_enable;

  // Prints all 8-bit general registers.
  printf("A: %02x, B: %02x, C: %02x, D: %02x, E: %02x, H: %02x, L: %02x, int_enable: %02x --- ", a, b, c, d, e, h, l, int_enable);

  // S, Z, 0, A, 0, P, 1, C         // Format of flags on Intel 8080 processor.
  uint8_t s = state->cc.s;          // Sign flag. Set to 1 if result is negative.
  uint8_t z = state->cc.z;          // Zero flag. Set to 1 if result is zero.
  uint8_t p = state->cc.p;          // Parity flag. Set to 1 if result is at parity.
  uint8_t cy = state->cc.cy;        // Carry flag. Set to 1 if arithmetic carry/borrow occured.
  uint8_t ac = state->cc.ac;        // Auxiliary carry flag (BCD operations).
  // Prints condition code flags
  printf("s: %d, z: %d, p: %d, cy: %d, ac: %d\n", s, z, p, cy, ac);
}

// Helper function to determine parity of a result using XOR method.
// Takes an 8 bit integer and returns a single bit.
// XOR operations reduce bits down to a single parity bit.
// 1 = even parity
// 0 = odd parity
// Code adapted from website linked below and adapted to C by Abraham Byun.
// https://www.freecodecamp.org/news/algorithmic-problem-solving-efficiently-computing-the-parity-of-a-stream-of-numbers-cd652af14643/ 
int parity(uint8_t num) {
  num = num ^ (num >> 4);  // XOR upper and lower 4 bits
  num = num ^ (num >> 2);  // XOR bits 2-3 with 0-1
  num = num ^ (num >> 1);  // XOR bit 1 with bit 0
  return !(num & 1);       // returns inverted lowest bit
}

// Helper function for setting the Zero, Sign, and Parity flags
// Takes state pointer and result of arithmetic operations (no return value)
// Sets Zero flag to 1 if result equals 0
// Sets Sign flag to 1 if bit 7 is set (negative in signed arithmetic)
// Sets Parity flag to 1 for even parity, 0 for odd parity
// Code adapted from Gemini.
void set_zsp_flags(State8080* state, uint8_t result) {
    state->cc.z = (result == 0);
    state->cc.s = ((result & 0x80) != 0);
    state->cc.p = parity(result);
}


// function for emulating 8080 cpu instruction execution
void Emulate8080Op(State8080* state, MachineState* machine) {
  // pointer to memory at program counter address position
  uint8_t* opcode = &state->memory[state->pc];
  //disassembled8080Op(state->memory, state->pc);

  switch(*opcode) {
    // NOP (No-operation)
    case 0x00: {
      state->pc += 1;
      break;
    }

    // LXI B,d16 (Load immediate register pair B & C)
    case 0x01: {
      state->b = opcode[2];
      state->c = opcode[1];
      state->pc+=3;
      break;
    }

    // STAX B (Store Accumulator into Memory BC)
    case 0x02: {
      // store accumulator register a into memory address at BC
      uint16_t address = (state->b << 8) | state->c;
      state->memory[address] = state->a;
      state->pc += 1;
      break;
    }

    // INX B (Increment Register Pair B-C)
    case 0x03: { 
      
      // Combine B and C into a single 16-bit value.
      uint16_t bc_pair = (state->b << 8) | state->c;
      
      // Increment the 16-bit value.
      bc_pair++;
      
      // Split the result back into the B and C registers.
      state->b = (bc_pair >> 8) & 0xFF;
      state->c = bc_pair & 0xFF;     

      state->pc += 1;
      break;
    }

    // INR B - Increment contents of register B
    // Updates Flags Z, S, P, AC
    case 0x04: {
      uint8_t result = state->b + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->b & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->b = result;
      state->pc += 1;
      break;
    }

    // DCR B - Decrement Register B
    // Updates Z, S, P, AC flags
    case 0x05: { 
      uint8_t result = state->b - 1;
      set_zsp_flags(state, result);
      // set AC flag if carry happens
      state->cc.ac = ((state->b & 0x0f) == 0);
      state->b = result;
      state->pc += 1;
      break;
    }
               
    // MVI B, d8 (Move Immediate 8-bit data to register B) 
    case 0x06: {
      state->b = opcode[1];
      state->pc += 2;
      break;
    }

    // RLC - Rotate A left, The low order bit and the CY flag are both set to the value shifted out of the high order bit position.
    // Updates CY flag
    case 0x07: { 
      uint8_t bit7 = (state->a >> 7) & 1;   // Extract bit 7 (MSB)
      state->a = (state->a << 1) | bit7;    // Shift LEFT, bit 7 becomes bit 0
      state->cc.cy = bit7;                  // Carry flag gets the old bit 7 value
      state->pc += 1;
      break;
    }

    // *NOP (No-operation - undocumented)
    case 0x08: {
      state-> pc += 1;
      break;
    }
      
    // DAD B - Add BC to HL
    // Updates CY flag
    case 0x09: {
      // combine register pairs into 16-bit values
      uint16_t bc = (state->b << 8) | state->c;
      uint16_t hl = (state->h << 8) | state->l;

      // perform 16-bit addition using 32-bit arithmetic to detect carry
      uint32_t result = (uint32_t)hl + (uint32_t)bc;

      // update carry flag - set if result exceeds 16 bits
      if (result > 0xFFFF) {
        state->cc.cy = 1; // set carry flag
      } else {
        state->cc.cy = 0; // clear carry flag
      }

      // store result back in HL register pair
      uint16_t final_result = result & 0xFFFF;
      state->h = (final_result >> 8) & 0xFF; // high byte to H
      state->l = final_result & 0xFF; // low byte to L

      state->pc += 1;
      break;
    }

    // *NOP (No-operation - undocumented)
    case 0x10: {
      state-> pc += 1;
      break;
    }

    // LDAX B
    case 0x0A: { 
        // Combine B and C registers to form a 16-bit memory address.
        uint16_t address = (state->b << 8) | state->c;
        
        // Load content of memory address into accumulator
        state->a = state->memory[address];
        
        state->pc += 1;
        break;
    }

    // DCX B (Decrement B & C)
    case 0x0B: {
      // decrement register pair bc
      uint16_t bc = (state->b << 8) | state->c;
      bc -= 1;

      // store high and low byte values
      state->b = (bc >> 8) & 0xff;
      state->c = bc & 0xff;
      
      state->pc += 1;
      break;
    }

    // INR C - Increment contents of register C
    // Updates Flags Z, S, P, AC
    case 0x0C: {
      uint8_t result = state->c + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->c & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->c = result;
      state->pc += 1;
      break;
    }

    // DCR C (decrement C register)
    // Update S, Z, A, P flgas
    case 0x0D: {
      uint8_t result = state->c - 1;

      set_zsp_flags(state, result);

      // set AC (auxillary carry flag)
      // AC flag set to 1 when carry did happen.
      state->cc.ac = ((state->c & 0x0f) == 0);

      state->c = result;
      state->pc += 1;

      break;
    }

    // MVI C, d8 (Move Immediate 8-bit data to register C) 
    case 0x0E:{
      state->c = opcode[1];
      state->pc += 2;
      break;
    } 
    
    // RRC - C - (Rotate A right, carry flag set to least significant bit)
    case 0x0F: { 
      uint8_t x = state->a;                 // x is not modified by bitwise operators.
      state->a = (x >> 1) | ((x & 1) << 7); // Bitwise OR 0111 1111 | 1000 0000 = 1111 1111
      state->cc.cy = (1 == (x & 1));        // Could it be just cc.cy = x & 1?
      state->pc += 1;
      break;
    }

    // LXI D,d16 (Load Register Pair D and E Immediate 16-bit Data)
    case 0x11: {
      state->d = opcode[2];
      state->e = opcode[1];
      state->pc +=3;
      break;
    }

    // STAX D (Store Accumulator into Memory DE)
    case 0x12: {
      // store accumulator register a into memory address at DE
      uint16_t address = (state->d << 8) | state->e;
      state->memory[address] = state->a;
      state->pc += 1;
      break;
    }

    // INX D (Increment D & E 16-bit register pair)
    case 0x13: {
      uint16_t de_pair = (state->d << 8) | state->e;
      
      de_pair++;

      state->d = (de_pair >> 8) & 0xFF; // The high byte goes back to D
      state->e = de_pair & 0xFF;        // The low byte goes back to E

      state->pc += 1;
      break;
    }

    // INR D - Increment contents of register D
    // Updates Flags Z, S, P, AC
    case 0x14: {
      uint8_t result = state->d + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->d & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->d = result;
      state->pc += 1;
      break;
    }

    // DCR D (decrement D register)
    // Update S, Z, A, P flgas
    case 0x15: {
      uint8_t result = state->d - 1;
      set_zsp_flags(state, result);

      // set AC (auxillary carry flag)
      // AC flag set to 1 when carry did happen.
      state->cc.ac = ((state->d & 0x0f) == 0);
      state->d = result;
      state->pc += 1;
      break;
    }
      
    // MVI D,d8 (Move Immediate 8-bit Data to Register D)
    case 0x16: {
      // load immediate next byte to register d
      state->d = opcode[1];

      state->pc += 2;
      break;
    }

    // *NOP (No-operation - undocumented)
    case 0x18: {
      state-> pc += 1;
      break;
    }

    // DAD D - Add DE to HL
    // updates Carry flag
    case 0x19: {
      // combine register pairs into 16-bit values
      uint16_t de = (state->d << 8) | state->e;
      uint16_t hl = (state->h << 8) | state->l;

      // perform 16-bit addition using 32-bit arithmetic to detect carry
      uint32_t result = (uint32_t)hl + (uint32_t)de;

      // update carry flag - set if result exceeds 16 bits
      if (result > 0xFFFF) {
        state->cc.cy = 1; // set carry flag
      } else {
        state->cc.cy = 0; // clear carry flag
      }

      // store result back in HL register pair
      uint16_t final_result = result & 0xFFFF;
      state->h = (final_result >> 8) & 0xFF; // high byte to H
      state->l = final_result & 0xFF; // low byte to L

      state->pc += 1;
      break;
    }

    // LDAX D (Load accumulator indirect from address in pair D and E)
    case 0x1A: { 
      uint16_t address = (state->d << 8) | (state->e);  // bitwise OR to turn two 8-bit addresses into one 16-bit address.
      state->a = state->memory[address];                // Register A now holds contents of that address.
      state->pc += 1;                                   // memory address is 16-bit, but contents of address are only 8-bits.
      break;
    }

    // DCX D (Decrement D & E)
    case 0x1B: {
      // decrement register pair de
      uint16_t de = (state->d << 8) | state->e;
      de -= 1;

      // store high and low byte values
      state->d = (de >> 8) & 0xff;
      state->e = de & 0xff;

      state->pc += 1;
      break;
    }

    // INR E - Increment contents of register E
    // Updates Flags Z, S, P, AC
    case 0x1C: {
      uint8_t result = state->e + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->e & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->e = result;

      state->pc += 1;
      break;
    }

    // DCR E (decrement E register)
    // Update S, Z, A, P flgas
    case 0x1D: {
      uint8_t result = state->e - 1;
      set_zsp_flags(state, result);
      
      // set AC (auxillary carry flag)
      // AC flag set to 1 when carry did happen.
      state->cc.ac = ((state->e & 0x0f) == 0);
      state->e = result;
      state->pc += 1;
      break;
    }
      
    // MVI E,d8 (Move Immediate 8-bit Data to Register E)
    case 0x1E: {
      // load immediate next byte to register e
      state->e = opcode[1];
      
      state->pc += 2;
      break;
    }

    // RAR - Rotate A right one position through the CY flag
    // Updates CY flag
    case 0x1F: { 
      uint8_t bit0 = state->a & 1;                        // Extract bit 0 (LSB)
      state->a = (state->a >> 1) | (state->cc.cy << 7);   // Shift right, put old carry in bit 7
      state->cc.cy = bit0;                                 // Carry gets old bit 0 value
      state->pc += 1;
      break;
    }

    // *NOP (No-operation - undocumented)
    case 0x20: {
      state-> pc += 1;
      break;
    }
    
    // LXI H,d16 (Load Register Pair H and L Immediate 16-bit Data)
    case 0x21: {
      state->h = opcode[2];
      state->l = opcode[1];
      state->pc +=3;
      break;
    }
    
    // SHLD a16 (Store H and L Direct)
    case 0x22: {
      // store l at a16 and h at a16+1 memory address
      uint16_t address = (opcode[2] << 8) | opcode[1];
      state->memory[address] = state->l;
      state->memory[address+1] = state->h;

      state->pc += 3;
      break;
    }

    // INX H (Increment HL 16-bit register pair)
    case 0x23: {
      uint16_t hl_pair = (state->h << 8) | state->l;
      
      hl_pair++;

      state->h = (hl_pair >> 8) & 0xFF; // The high byte goes back to H
      state->l = hl_pair & 0xFF;        // The low byte goes back to L

      state->pc += 1;
      break;
    }

    // INR H - Increment contents of register H
    // Updates Flags Z, S, P, AC
    case 0x24: {
      uint8_t result = state->h + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->h & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->h = result;
      state->pc += 1;
      break;
    }

    // DCR H (decrement H register)
    // Update S, Z, A, P flgas
    case 0x25: {
      uint8_t result = state->h - 1;
      set_zsp_flags(state, result);
      
      // set AC (auxillary carry flag)
      // AC flag set to 1 when carry did happen.
      state->cc.ac = ((state->h & 0x0f) == 0);
      state->h = result;
      state->pc += 1;
      break;
    }

    // MVI H (Load immediate 8-bit data to register H)
    case 0x26: {
      state->h = opcode[1];   // Load register H with contents of byte 2; 
      state->pc += 2;
      break;
    }

    // DAA - Decimal Adjust Accumulator
    // All flags are affected
    /**
     * The Intel 8080 Users Manual States: 
     * The 8-bit number in the accumulator is adjusted to form two four-bit 
     * Binary-Coded-Decimal digits by the following process:
     * 1. If the value of the least significant 4 bits of the accumulator is greater than 9, 
     *    or if the AC flag is set, 6 is added to the accumulator
     * 2. If the value of the most significant 4 bits of the accumulator is now greater than 9, 
     *    or if the CY flag is set, 6 is added to the most significant 4 bits of the accumulator
     */
    case 0x27: {
      uint8_t original_a = state->a;
      uint8_t original_cy = state->cc.cy;
      uint16_t result = original_a;

      // Step 1: Adjust lower nibble and set Auxiliary Carry
      bool new_ac = ((original_a & 0x0F) > 9) || (state->cc.ac == 1);
      if (new_ac) {
        result += 6;
      }

      // Step 2: Adjust upper nibble and set Carry
      bool new_cy = (((result >> 4) & 0x0F) > 9) || (original_cy == 1);
       if (new_cy) {
        result += 0x60;
      }

      // Set all the flags based on the final result
      state->a = result & 0xFF;
      set_zsp_flags(state, state->a);
      state->cc.cy = new_cy;
      state->cc.ac = new_ac;

      state->pc += 1;
      break;
    }

    // *NOP (No-operation - undocumented)
    case 0x28: {
      state-> pc += 1;
      break;
    }
     
    // DAD H - Add HL to HL
    // updates Carry flag
    case 0x29: {
      // combine register pair into 16-bit values
      uint16_t hl = (state->h << 8) | state->l;

      // perform 16-bit addition using 32-bit arithmetic to detect carry
      uint32_t result = (uint32_t)hl + (uint32_t)hl;

      // update carry flag - set if result exceeds 16 bits
      if (result > 0xFFFF) {
        state->cc.cy = 1; // set carry flag
      } else {
        state->cc.cy = 0; // clear carry flag
      }

      // store result back in HL register pair
      uint16_t final_result = result & 0xFFFF;
      state->h = (final_result >> 8) & 0xFF; // high byte to H
      state->l = final_result & 0xFF; // low byte to L

      state->pc += 1;
      break;
    }

    // LHLD a16 (Load H and L Direct)
    case 0x2A: {
      // retrieve l from a16 and h at a16+1 memory address
      uint16_t address = (opcode[2] << 8) | opcode[1];
      state->l = state->memory[address];
      state->h = state->memory[address+1];
      
      state->pc += 3;
      break;
    }

    // DCX H (Decrement Register Pair H-L)
    case 0x2B: { 

      // combine H and L
      uint16_t hl_pair = (state->h << 8) | state->l;

      hl_pair--;

      // store result back in HL register pair
      state->h = (hl_pair >> 8) & 0xFF; // high byte goes to H
      state->l = hl_pair & 0xFF;        // low byte goes to L

      state->pc += 1;
      break;
    }

    // INR L - Increment contents of register L
    // Updates Flags Z, S, P, AC
    case 0x2C: {
      uint8_t result = state->l + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->l & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->l = result;
      state->pc += 1;
      break;
    }

    // MVI L, d8 (Move Immediate to L)
    case 0x2E: { 

      // Load the 'l' register with the immediate data
      state->l = opcode[1];

      state->pc += 2;
      break;
    }

    // CMA - Complement Accumulator
    // No flags affected
    case 0x2F: {
      state->a = ~state->a;    // Bitwise NOT - flip all bits
      state->pc += 1;
      break;
    }
      
    // *NOP (No-operation - undocumented)
    case 0x30: {
      state-> pc += 1;
      break;
    }
    
    // LXI SP,d16 (Load Stack Pointer Immediate 16-bit Data)
    case 0x31: {
      // shift high byte left 8 bits, then OR with low byte to make 
      state->sp = (opcode[2]<<8) | opcode[1];  
      state->pc +=3;
      break;
    }

    // STA a16 (store accumulator direct) 
    case 0x32: {
      uint16_t address = (opcode[2] << 8) | (opcode[1]);    // Create memory address from bytes 3 and 2.
      state->memory[address] = state->a;
      state->pc +=3;
      break;
    }

    // INR M - Increment content of memory location whose address is contained in H and L registers
    // Updates Flags Z, S, P, AC
    case 0x34: {
      uint16_t address = (state->h << 8) | (state->l); // create memory address from registers h and l
      uint8_t original = state->memory[address];
      uint8_t result = original + 1;  // increment by 1
      state->memory[address] = result;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((original & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->pc += 1;
      break;
    }

    // DCR M (Decrement content of memory location whose address is contained in H and L registers)
    case 0x35: {
      uint16_t address = (state->h << 8) | (state->l); // create memory address from registers h and l
      uint8_t original = state->memory[address];
      uint8_t result = original - 1;   // decrement by 1
      state->memory[address] = result;
      set_zsp_flags(state, result);
      // set AC flag if carry happens
      state->cc.ac = ((original & 0x0f) == 0);
      state->pc += 1;
      break;
    }

    // MVI M (Load immediate 8-bit data to registers H and L)           
    case 0x36: {
      uint16_t address = (state->h << 8) | (state->l);  // Create memory address from registers h and l.
      state->memory[address] = opcode[1];               // Move immediate 8-bit data to that address.
      state->pc += 2;
      break;
    }

    // STC (Set Carry)
    case 0x37: { 
      // Set the Carry flag to 1.
      state->cc.cy = 1;
      
      state->pc += 1;
      break;
    }
    
    // *NOP (No-operation - undocumented)
    case 0x38: {
      state-> pc += 1;
      break;
    }

    // DAD SP (Add stack pointer to H & L)
    // Updates carry flag
    case 0x39: {
      // combine register pair into 16-bit value
      uint16_t hl = (state->h << 8) | state->l;

      // 16-bit addition using 32-bit typecast to detect overflow
      uint32_t result = (uint32_t)hl + (uint32_t)state->sp;

      // update carry flag if result exceeds 16-bits
      state->cc.cy = (result > 0xFFFF);

      // mask lower 16-bits and store result back into HL register pair
      state->h = (result >> 8) & 0xFF;  // high byte to H
      state->l = result & 0xFF;  // low byte to L

      state->pc += 1;
      break;
    }

    // LDA (Load accumulator direct 16-bit data)
    case 0x3A: {
      uint16_t address = (opcode[2] << 8) | (opcode[1]);    // Create memory address from bytes 3 and 2.
      state->a = state->memory[address];                    // Load register a with contents of memory address.
      state->pc += 3;
      break;
    }

    // INR A - Increment contents of register A
    // Updates Flags Z, S, P, AC
    case 0x3C: {
      uint8_t result = state->a + 1;
      set_zsp_flags(state, result);
      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0f) == 0x0f);  // Check if lower 4 bits are all 1s
      state->a = result;
      state->pc += 1;
      break;
    }

    // DCR A - Decrement Register A
    // Updates Z, S, P, AC flags
    case 0x3D: { 
      uint8_t result = state->a - 1;
      set_zsp_flags(state, result);
      // set AC flag if carry happens
      state->cc.ac = ((state->a & 0x0f) == 0);
      state->a = result;
      state->pc += 1;
      break;
    }

    // MVI A (Move immediate 8-bit data to Accumulator)
    case 0x3E: {
      state->a = opcode[1];   // Load register A with contents of byte 2;
      state->pc += 2;
      break;
    }

    // CMC - Complement Carry Flag
    // CY flag affected
    case 0x3F: {
      state->cc.cy = ~state->cc.cy;    // Bitwise NOT - flip CY bit
      state->pc += 1;
      break;
    }

    // MOV B,B
    case 0x40: {
      state->b = state->b;
      state->pc += 1;
      break;
    }

    // MOV B,C
    case 0x41: {
      state->b = state->c;
      state->pc += 1;
      break;
    }

    // MOV B,D
    case 0x42: {
      state->b = state->d;
      state->pc += 1;
      break;
    }

    // MOV B,H
    case 0x44: {
      state->b = state->h;
      state->pc += 1;
      break;
    }
    
    // MOV B,L
    case 0x45: {
      state->b = state->l;
      state->pc += 1;
      break;
    }      

    // MOV B, M (Move from Memory to B)
    case 0x46: { 

      // memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      
      state->b = state->memory[address];

      state->pc += 1;
      break;
    }

    // MOV B,A
    case 0x47: {
      state->b = state->a;
      state->pc += 1;
      break;
    }
    
    // MOV C,B
    case 0x48: {
      state->c = state->b;
      state->pc += 1;
      break;
    }
    
    // MOV C,C
    case 0x49: {
      state->c = state->c;
      state->pc += 1;
      break;
    }

    // MOV C,D
    case 0x4A: {
      state->c = state->d;
      state->pc += 1;
      break;
    }

    // MOV C,E
    case 0x4B: {
      state->c = state->e;
      state->pc += 1;
      break;
    }

    // MOV C,H
    case 0x4C: {
      state->c = state->h;
      state->pc += 1;
      break;
    }
    
    // MOV C,L
    case 0x4D: {
      state->c = state->l;
      state->pc += 1;
      break;
    }

    // MOV C,M
    case 0x4E: {
      uint16_t address = (state->h << 8) | (state->l);
      state->c = state->memory[address];
      state->pc += 1;
      break;  
    }
    
    // MOV C, A (Move from Accumulator to C)
    case 0x4F: { 

      // Copy a reg to c reg
      state->c = state->a;

      state->pc += 1;
      break;
    }

    // MOV D,B
    case 0x50: {
      state->d = state->b;
      state->pc += 1;
      break;
    }

    // MOV D,C
    case 0x51: {
      state->d = state->c;
      state->pc += 1;
      break;
    }

    // MOV D,H
    case 0x54: {
      state->d = state->h;
      state->pc += 1;
      break;
    }

    // MOV D,M - Move Data from Memory (addressed by H and L) to Register D
    case 0x56: {
      // reconstruct 16-bit address
      uint16_t address = (state->h << 8) | (state->l);
      state->d = state->memory[address];
      state->pc += 1;
      break;
    }

    // MOV D, A (Move from Accumulator to D)
    case 0x57: { 
      state->d = state->a;
      state->pc += 1;
      break;
    }

        // MOV E,C
    case 0x59: {
      state->e = state->c;
      state->pc += 1;
      break;
    }
    
    // MOV E,E
    case 0x5B: {
      state->e = state->e;
      state->pc += 1;
      break;
    }

    // MOV E,M (Move Data from Memory (addressed by H and L) to Register E)
    case 0x5E: {
      // access 16-bit memory address at HL register pair
      state->e = state->memory[(state->h <<8 | state->l)];
      state->pc += 1;
      break;
    }

    // MOV E, A (Move from Accumulator to E)
    case 0x5F: { 
      state->e = state->a;
      state->pc += 1;
      break;
    }

    // MOV H,B
    case 0x60: {
      state->h = state->b;
      state->pc += 1;
      break;
    }

    // MOV H,C
    case 0x61: {
      state->h = state->c;
      state->pc += 1;
      break;
    }

    // MOV H,D
    case 0x62: {
      state->h = state->d;
      state->pc += 1;
      break;
    }

    // MOV H,E
    case 0x63: {
      state->h = state->e;
      state->pc += 1;
      break;
    }

    // MOV H,H
    case 0x64: {
      state->h = state->h;
      state->pc += 1;
      break;
    }

    // MOV H,L
    case 0x65: {
      state->h = state->l;
      state->pc += 1;
      break;
    }

    // MOV H, M (Move data from memory to H)
    case 0x66: {
      uint16_t address = (state->h<<8) | (state->l);
      state->h = state->memory[address];
      state->pc += 1;
      break;
    }

    // MOV H, A (Move from Accumulator to H)
    case 0x67: { 
      
      state->h = state->a;
      
      state->pc += 1;
      break;
    }

    // MOV L,B (Move data from b to register l)
    case 0x68: {
      state->l = state->b;
      state->pc += 1;
      break;
    }

    // MOV L,C (Move data from c to register l)
    case 0x69: {
      state->l = state->c;
      state->pc += 1;
      break;
    }

    // MOV L,H
    case 0x6C: {
      state->l = state->h;
      state->pc += 1;
      break;
    }

    // MOV L,L
    case 0x6D: {
      state->l = state->l;
      state->pc += 1;
      break;
    }
    // MOV L,M
    case 0x6E: {
      uint16_t address = (state->h << 8) | state->l;
      state->l = state->memory[address];
      state->pc += 1;
      break;
    }

    // MOV L,A (Move data from accumulator to register l)
    case 0x6F: {
      state->l = state->a;
      state->pc += 1;
      break;
    }
    
    // MOV M,B
    case 0x70: {
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->b;
      state->pc += 1;
      break;
    }

    // MOV M,C
    case 0x71: {
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->c;
      state->pc += 1;
      break;
    }

    // MOV M,D
    case 0x72: {
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->d;
      state->pc += 1;
      break;
    }

    // MOV M,E
    case 0x73: {
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->e;
      state->pc += 1;
      break;
    }

    // MOV M,H
    case 0x74: {
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->h;
      state->pc += 1;
      break;
    }

    // HLT
    case 0x76: {
      exit(0);
      break;
    }

    // MOV M,A - Move Data from Accumulator to Memory (addressed by H and L)
    case 0x77: {
      // reconstruct 16-bit address
      uint16_t address = (state->h << 8) | (state->l);
      state->memory[address] = state->a;
      state->pc += 1;
      break;
    }

    // MOV A, B (Move from B to Accumulator)
    case 0x78: { 

      //Copy from b reg to a reg.
      state->a = state->b;

      state->pc += 1;
      break;
    }
    
    // MOV A, C (Move from C to Accumulator)
    case 0x79: { 

      //Copy from c reg to a reg.
      state->a = state->c;

      state->pc += 1;
      break;
    }
    
    // MOV A,D (Move Data from Register D to Accumulator)
    case 0x7A: {
      state->a = state->d;
      state->pc += 1;
      break;
    }
    
    // MOV A, E (Move data from register E to accumulator)
    case 0x7B: {
      state->a = state->e;
      state->pc += 1;
      break;
    }

    // MOV A,H (Move data from register h to accumulator)
    case 0x7C: {
      state->a = state->h;
      state->pc += 1;
      break;
    }

    // MOV A, L (Move from L to Accumulator)
    case 0x7D: { 
      // Copy from L reg to a reg.
      state->a = state->l;
      state->pc += 1;
      break;
    }
    
    // MOV A,M (Move Data from Memory (addressed by H and L) to Accumulator)
    case 0x7E: {
      state->a = state->memory[(state->h << 8 | state->l)];
      state->pc += 1;
      break;
    }

    // MOV A,A (Move from A to A)
    case 0x7F: {
      state->a = state->a;  // NOP
      state->pc += 1;
      break;
    }

    // ADD B - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x80: {
      uint8_t addend = state->b;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD C - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x81: {
      uint8_t addend = state->c;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD D - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x82: {
      uint8_t addend = state->d;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD E - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x83: {
      uint8_t addend = state->e;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD H - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x84: {
      uint8_t addend = state->h;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD L - Content of register is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x85: {
      uint8_t addend = state->l;
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADD M - Content of memory at address HL is added to content of the accumulator
    // Updates flags Z, S, P, CY, AC
    case 0x86: {
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      
      // get the byte from that memory location
      uint8_t addend = state->memory[address];
      
      uint16_t result = state->a + addend; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend & 0x0F)) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADC B - Content of register is added to content of the accumulator along with CY flag (A = A + r + CY)
    // Updates flags Z, S, P, CY, AC
    case 0x88: {
      uint8_t addend1 = state->b;
      uint8_t addend2 = state->cc.cy;
      uint16_t result = state->a + addend1 + addend2; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend1 & 0x0F) + addend2) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADC D - Content of register is added to content of the accumulator along with CY flag (A = A + r + CY)
    // Updates flags Z, S, P, CY, AC
    case 0x8A: {
      uint8_t addend1 = state->d;
      uint8_t addend2 = state->cc.cy;
      uint16_t result = state->a + addend1 + addend2; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend1 & 0x0F) + addend2) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADC E - Content of register is added to content of the accumulator along with CY flag (A = A + r + CY)
    // Updates flags Z, S, P, CY, AC
    case 0x8B: {
      uint8_t addend1 = state->e;
      uint8_t addend2 = state->cc.cy;
      uint16_t result = state->a + addend1 + addend2; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend1 & 0x0F) + addend2) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // ADC M - Content of memory at address HL is added to content of the accumulator along with CY flag (A = A + HL + CY)
    // Updates flags Z, S, P, CY, AC
    case 0x8E: {
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      
      // get the byte from that memory location
      uint8_t addend1 = state->memory[address];
      uint8_t addend2 = state->cc.cy;
      
      uint16_t result = state->a + addend1 + addend2; // use 16-bit type to detect carry
      set_zsp_flags(state, result & 0xFF);   // mask result back to 8 bit, set_zsp_flags expects uint8_t
      state->cc.cy = (result >> 8) & 1;

      // AC is set if there's a carry from bit 3 to bit 4
      state->cc.ac = ((state->a & 0x0F) + (addend1 & 0x0F) + addend2) > 0x0F;

      state->a = result & 0xFF;              // mask result back to 8-bit
      state->pc += 1;
      break;
    }

    // SUB B - Content of register is subtracted from content of accumulator (A = A - r)
    // Updates flags Z, S, P, CY, AC
    case 0x90: {
      uint8_t subtrahend = state->b;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);

      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SUB H - Content of register is subtracted from content of accumulator (A = A - r)
    // Updates flags Z, S, P, CY, AC
    case 0x94: {
      uint8_t subtrahend = state->h;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SUB A - Content of register is subtracted from content of accumulator (A = A - r)
    // Updates flags Z, S, P, CY, AC
    case 0x97: {
      uint8_t subtrahend = state->a;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB B - Content of register and CY flag is subtracted from content of accumulator (A = A - r - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x98: {
      uint8_t subtrahend1 = state->b;
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB C - Content of register and CY flag is subtracted from content of accumulator (A = A - r - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x99: {
      uint8_t subtrahend1 = state->c;
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB D - Content of register and CY flag is subtracted from content of accumulator (A = A - r - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x9A: {
      uint8_t subtrahend1 = state->d;
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB E - Content of register and CY flag is subtracted from content of accumulator (A = A - r - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x9B: {
      uint8_t subtrahend1 = state->e;
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB L - Content of register and CY flag is subtracted from content of accumulator (A = A - r - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x9D: {
      uint8_t subtrahend1 = state->l;
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // SBB M - Content of memory at address HL and CY flag is subtracted from content of accumulator (A = A - HL - CY)
    // Updates flags Z, S, P, CY, AC
    case 0x9E: {
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;

      uint8_t subtrahend1 = state->memory[address];
      uint8_t subtrahend2 = state->cc.cy;
      uint8_t result = state->a - subtrahend1 - subtrahend2; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < (r + CY) (need to borrow)
      state->cc.cy = (state->a < (subtrahend1 + subtrahend2));
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < ((subtrahend1 & 0x0F) + subtrahend2);

      state->a = result;
      state->pc += 1;
      break;
    }

    // ANA B - Logical AND register with Accumulator
    // updates Z, S, P, CY, AC
    case 0xA0: {
      uint8_t operand1 = state->a;
      uint8_t operand2 = state->b;
      uint8_t result = operand1 & operand2;

      set_zsp_flags(state, result);

      // AC: Logical OR of bit 3 of both operands per 8080 Programmer Manual
      state->cc.ac = ((operand1 & 0x08) | (operand2 & 0x08)) != 0;
      
      // always reset CY flag for logical operations
      state->cc.cy = 0;

      state->a = result;
      state->pc += 1;
      break;
    }

    // ANA E - Logical AND register with Accumulator
    // updates Z, S, P, CY, AC
    case 0xA3: {
      uint8_t operand1 = state->a;
      uint8_t operand2 = state->e;
      uint8_t result = operand1 & operand2;

      set_zsp_flags(state, result);

      // AC: Logical OR of bit 3 of both operands per 8080 Programmer Manual
      state->cc.ac = ((operand1 & 0x08) | (operand2 & 0x08)) != 0;
      
      // always reset CY flag for logical operations
      state->cc.cy = 0;

      state->a = result;
      state->pc += 1;
      break;
    }

    // ANA M - Logical AND contents at address HL with Accumulator
    // updates Z, S, P, CY, AC
    case 0xA6: {
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      uint8_t operand1 = state->a;
      uint8_t operand2 = state->memory[address];
      uint8_t result = operand1 & operand2;

      set_zsp_flags(state, result);

      // AC: Logical OR of bit 3 of both operands per 8080 Programmer Manual
      state->cc.ac = ((operand1 & 0x08) | (operand2 & 0x08)) != 0;
      
      // always reset CY flag for logical operations
      state->cc.cy = 0;

      state->a = result;
      state->pc += 1;
      break;
    }

    // ANA A - Logical AND Accumulator with Accumulator
    // updates Z, S, P, CY, AC
    case 0xA7: {
      uint8_t operand1 = state->a;
      uint8_t operand2 = state->a; // same for ANA A
      uint8_t result = operand1 & operand2;

      set_zsp_flags(state, result);

      // AC: Logical OR of bit 3 of both operands per 8080 Programmer Manual
      state->cc.ac = ((operand1 & 0x08) | (operand2 & 0x08)) != 0;
      
      // always reset CY flag for logical operations
      state->cc.cy = 0;

      state->a = result;
      state->pc += 1;
      break;
    }

    // XRA B (XOR Accumulator with B)
    case 0xA8: { 

      // Perform the bitwise XOR between the accumulator and register B.
      uint8_t result = state->a ^ state->b;

      set_zsp_flags(state, result);

      // All logical XOR instructions clear the Carry and Aux Carry flags.
      state->cc.cy = 0;
      state->cc.ac = 0;

      state->a = result;

      state->pc += 1;
      break;
    }

    // XRA D - Exclusive OR Accumulator with register
    // updates Z, S, P, CY, AC
    case 0xAA: {
      uint8_t result = state->a ^ state->d;  // A XOR D
      
      // Set Z, S, P flags based on result
      set_zsp_flags(state, result);
      
      // CY and AC flags are explicitly cleared for XRA instructions
      state->cc.cy = 0;
      state->cc.ac = 0;
      
      state->a = result;
      state->pc += 1;
      break;
    }

    // XRA A - Exclusive OR Accumulator with Accumulator
    // updates Z, S, P, CY, AC
    case 0xAF: {
      uint8_t result = state->a ^ state->a;  // A XOR A
      
      // Set Z, S, P flags based on result
      set_zsp_flags(state, result);
      
      // CY and AC flags are explicitly cleared for XRA instructions
      state->cc.cy = 0;
      state->cc.ac = 0;
      
      state->a = result;
      state->pc += 1;
      break;
    }

    // ORA B (OR Accumulator with B)
    case 0xB0: { 

      // Perform the bitwise OR between the accumulator and register B.
      uint8_t result = state->a | state->b;

      set_zsp_flags(state, result);

      // All logical OR instructions clear the Carry and Aux Carry flags.
      state->cc.cy = 0;
      state->cc.ac = 0;

      state->a = result;

      state->pc += 1;
      break;
    }

    // ORA E - OR Accumulator with register
    // updates Z, S, P, CY, AC
    case 0xB3: { 

      // Perform the bitwise OR between the accumulator and register
      uint8_t result = state->a | state->e;

      set_zsp_flags(state, result);

      // All logical OR instructions clear the Carry and Aux Carry flags.
      state->cc.cy = 0;
      state->cc.ac = 0;

      state->a = result;

      state->pc += 1;
      break;
    }

    // ORA H - OR Accumulator with register
    // updates Z, S, P, CY, AC
    case 0xB4: { 

      // Perform the bitwise OR between the accumulator and register
      uint8_t result = state->a | state->h;

      set_zsp_flags(state, result);

      // All logical OR instructions clear the Carry and Aux Carry flags.
      state->cc.cy = 0;
      state->cc.ac = 0;

      state->a = result;

      state->pc += 1;
      break;
    }

    // ORA M (OR Accumulator with Memory)
    case 0xB6: { 
      
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      
      // get the byte from that memory location
      uint8_t operand = state->memory[address];
      
      // bitwise OR between the accumulator and the memory byte.
      uint8_t result = state->a | operand;
      
      // Set flags
      set_zsp_flags(state, result);
      
      // Logical OR instructions ALWAYS clear the Carry and Aux Carry flags.
      state->cc.cy = 0;
      state->cc.ac = 0;
      
      // final result back into the accumulator.
      state->a = result;
      
      state->pc += 1;
      break;
    }

    // CMP B - Content of register is compared (subtracted) from content of accumulator (A = A - r)
    // The accumulator is unchanged. The condition flags are set as a result of the subtraction.
    // Updates flags Z, S, P, CY, AC
    case 0xB8: {
      uint8_t subtrahend = state->b;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);
      state->pc += 1;
      break;
    }

    // CMP E - Content of register is compared (subtracted) from content of accumulator (A = A - r)
    // The accumulator is unchanged. The condition flags are set as a result of the subtraction.
    // Updates flags Z, S, P, CY, AC
    case 0xBB: {
      uint8_t subtrahend = state->e;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);
      state->pc += 1;
      break;
    }

    // CMP H - Content of register is compared (subtracted) from content of accumulator (A = A - r)
    // The accumulator is unchanged. The condition flags are set as a result of the subtraction.
    // Updates flags Z, S, P, CY, AC
    case 0xBC: {
      uint8_t subtrahend = state->h;
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);
      state->pc += 1;
      break;
    }

    // CMP M - Content of memory at address HL is compared (subtracted) from content of accumulator (A = A - HL)
    // The accumulator is unchanged. The condition flags are set as a result of the subtraction.
    // Updates flags Z, S, P, CY, AC
    case 0xBE: {
      // get the memory address from the H-L register pair.
      uint16_t address = (state->h << 8) | state->l;
      uint8_t subtrahend = state->memory[address];
      uint8_t result = state->a - subtrahend; // can use 8-bit 
      set_zsp_flags(state, result);

      // CY is set if A < r (need to borrow)
      state->cc.cy = (state->a < subtrahend);
      
      // AC (Auxiliary Carry/Borrow): Set if lower 4 bits need to borrow
      state->cc.ac = (state->a & 0x0F) < (subtrahend & 0x0F);
      state->pc += 1;
      break;
    }

    // RNZ (Return if Not Zero)
    case 0xC0: { 

      // Check if the Zero flag is clear.
      if (state->cc.z == 0) {
        // do return
        uint8_t pcl = state->memory[state->sp];
        uint8_t pch = state->memory[state->sp + 1];
        
        // reconstruct the full 16-bit address
        uint16_t return_address = (pch << 8) | pcl;

        // set the program counter equal return address.
        state->pc = return_address;
        
        // adjust the stack pointer because we popped two bytes.
        state->sp += 2;
        
      } else {
        // no return since flag is set, so it is zero
        state->pc += 1;
      }
      break;
    }

    // POP B (Pop off stack to register pairs b & c)
    case 0xC1: {
      state->b = state->memory[state->sp + 1];    // B = Contents of sp + 1.
      state->c = state->memory[state->sp];        // C = Contents of sp.
      state->sp += 2;                             // Increment sp by 2.
      state->pc += 1;
      break;
    }

    // JNZ a16 - Jump if not zero direct
    // no flags affected
    case 0xC2: {
      // check if zero flag is clear (0 = not zero, 1 = zero)
      if (state->cc.z == 0) {
        // jump taken: reconstruct 16-bit address from little endian bytes
        uint16_t address = (opcode[2] << 8) | (opcode[1]); // high byte | low byte
        state->pc = address;
      } else {
        // jump not taken: skip over 3-byte instruction (opcode + 2 addres bytes)
        state->pc += 3;
      }
      break;
    }

    // JMP a16 (Jump Direct)
    case 0xC3: {
      // set program counter to 16-bit memory address
      state->pc = (opcode[2] << 8 | opcode[1]);
      break;
    }

    // CNZ a16 (Call on no zero)
    case 0xC4: {
      // call subroutine at a16 if no zero (zero flag = 0)
      if (state->cc.z == 0) {
        // get 16-bit memory address and save next pc 
        uint16_t address = (opcode[2] << 8) | opcode[1];
        uint16_t pc_return = state-> pc + 3;

        // push return address onto stack
        state->memory[state->sp - 1] = (pc_return >> 8) & 0xff;  // high byte
        state->memory[state->sp - 2] = pc_return & 0xff; // low byte
        state->sp -= 2;

        // jump to target address
        state->pc = address;
      } else {
        state->pc += 3;
      }

      break;
    }

    // PUSH B (Push register pair B & C on stack)
    case 0xC5: {
      state->memory[state->sp - 1] = state->b;
      state->memory[state->sp - 2] = state->c;

      state->sp = state->sp - 2;
      state->pc += 1;
      break;
    }

    // ADI - S, Z, A, P, C - (Add immediate 8-bit data to accumulator) 
    // code adapted from emulator101 - arithmetic group page
    case 0xC6: {
      uint16_t result = state->a + opcode[1];     // Using 16-bit to detect if carry took place.
      state->cc.s = ((result & 0x80) != 0);       // Logical AND with 1000 0000 to check sign bit.
      state->cc.z = ((result & 0xff) == 0);       // Logical AND with 1111 1111 to check if result is 0.
      state->cc.p = parity(result);               // Check parity using helper function.
      state->cc.cy = (result > 0xff);             // Check if result is greater than 1111 1111.
      state->cc.ac = ((state->a & 0x0f) + 
          (opcode[1] & 0x0f)) > 0x0f;             // AC flag not used in Space Invaders.
      state->a = result & 0xff;                   // Logical AND with 1111 1111 to mask back to 8-bits.
      state->pc += 2;
      break;
    }

    // RZ - Return if conditional is true: Z flag = 1
    case 0xC8: {
      if (state->cc.z == 1) {
        // read return address from stack (little endian)
        uint8_t pcl = state->memory[state->sp]; // low byte from stack
        uint8_t pch = state->memory[state->sp + 1]; // high byte from stack

        // reconstruct 16-bit address
        uint16_t address = (pch << 8) | (pcl);

        // jump to return address and adjust stack pointer
        state->pc = address;
        state->sp += 2;

        // DOES NOT INCREASE PC BY 1
        // RET is a control flow instruction that sets the PC to a completely new address from the stack. 
        // The return address already points to the correct next instruction to execute.
      } else {
        // condition false: continue sequentially
        state->pc += 1;
      }
      break;
    }

    case 0xC9: {
      // read return address from stack (little endian)
      uint8_t pcl = state->memory[state->sp]; // low byte from stack
      uint8_t pch = state->memory[state->sp + 1]; // high byte from stack

      // reconstruct 16-bit address
      uint16_t address = (pch << 8) | (pcl);

      // jump to return address and adjust stack pointer
      state->pc = address;
      state->sp += 2;

      // DOES NOT INCREASE PC BY 1
      // RET is a control flow instruction that sets the PC to a completely new address from the stack. 
      // The return address already points to the correct next instruction to execute.
      break;
    }

    // JZ a16 - Jump if Z=1
    // no flags affected
    case 0xCA: {
      // check if zero flag is set (0 = clear, 1 = set)
      if (state->cc.z == 1) {
        // jump taken: reconstruct 16-bit address from little endian bytes
        uint16_t address = (opcode[2] << 8) | (opcode[1]); // high byte | low byte
        state->pc = address;
      } else {
        // jump not taken: skip over 3-byte instruction (opcode + 2 addres bytes)
        state->pc += 3;
      }
      break;
    }

    // CZ a16 (Call on zero)
    case 0xCC: {
      // call subroutine at a16 if zero (zero flag == 1)
      if (state->cc.z == 1) {
        // get 16-bit memory address and save next pc 
        uint16_t address = (opcode[2] << 8) | opcode[1];
        uint16_t pc_return = state-> pc + 3;

        // push return address onto stack
        state->memory[state->sp - 1] = (pc_return >> 8) & 0xff;  // high byte
        state->memory[state->sp - 2] = pc_return & 0xff; // low byte
        state->sp -= 2;

        // jump to target address
        state->pc = address;
      } else {
        state->pc += 3;
      }

      break;
    }
    
    // CALL a16 (Call Subroutine Direct)
    case 0xCD: 
    {
      // (subroutine) call address and return (next instruction) addresses
      uint16_t call_address = (opcode[2] << 8 | opcode[1]);
      uint16_t return_address = state->pc+3;
      
      // push return address bytes to stack (later read by RET instruction)
      // reverse isolated high-low byte order so low byte is popped first (little endian)
      state->memory[state->sp-1] = ((return_address >> 8) & 0xff);  // high byte (shift right, 8-bit bitwise AND)
      state->memory[state->sp-2] = (return_address & 0xff);  // low byte (8-bit bitwise AND)
      state->sp -= 2;

      // jump to subbroutine call address
      state->pc = call_address;

      break;
    }

    // RNC (Return if No Carry)
    case 0xD0: { 

      // Check if the Carry flag (cy) is clear (equal to 0).
      if (state->cc.cy == 0) {
        // do return
        // Pop the 16-bit return address from the stack.
        uint8_t pcl = state->memory[state->sp];
        uint8_t pch = state->memory[state->sp + 1];
        
        // Reconstruct the full 16-bit address.
        uint16_t return_address = (pch << 8) | pcl;

        state->pc = return_address;
        
        state->sp += 2;
        
      } else {
        // don't do return
        state->pc += 1;
      }
      break;
    }

    // POP D (Pop register pair D & E off stack)
    case 0xD1: {
      state->d = state->memory[state->sp + 1];
      state->e = state->memory[state->sp];
      
      state->sp = state->sp + 2;
      state->pc += 1;
      break;
    }

    // JNC addr (Jump if No Carry)
    case 0xD2: { 
      
      // Check if the Carry flag (cy) is clear.
      if (state->cc.cy == 0) {
        // the 16-bit jump address from the next two bytes.
        uint16_t jmp_address = (opcode[2] << 8) | opcode[1];
        
        // Set the program counter to the new address.
        state->pc = jmp_address;
      } else {
        // if no jump
        state->pc += 3;
      }
      break;
    }

    // OUT (Output immediate 8-bit to port)
    case 0xD3: {
      //port info from: https://www.computerarcheology.com/Arcade/SpaceInvaders/Hardware.html
      
      uint8_t port_number = opcode[1];
      uint8_t value = state->a; // The accumulator holds the data

      switch (port_number) {
        case 2: // Set shift amount
            // The hardware only uses the lower 3 bits for the offset
            machine->shift_offset = value & 0x7; 
            break;
        
        case 3: { // Sound 1
            // These are edge-triggered; the sound plays when the bit becomes 1.
            if (value & 0x01) sound_play(SOUND_UFO);
            if (value & 0x02) sound_play(SOUND_SHOT);
            if (value & 0x04) sound_play(SOUND_PLAYER_DIE);
            if (value & 0x08) sound_play(SOUND_INVADER_DIE);
            break;
        }

        case 4: { // Load shift register
            // The new value becomes the HIGH byte of the 16-bit register,
            // and the existing HIGH byte becomes the new LOW byte.
            // This simulates the data shifting through the register.
            uint8_t old_hi = machine->shift_register >> 8;
            machine->shift_register = (value << 8) | old_hi;
            break;
        }

        case 5: { // Sound 2
            if (value & 0x01) sound_play(SOUND_FLEET_1);
            if (value & 0x02) sound_play(SOUND_FLEET_2);
            if (value & 0x04) sound_play(SOUND_FLEET_3);
            if (value & 0x08) sound_play(SOUND_FLEET_4);
            if (value & 0x10) sound_play(SOUND_UFO_HIT);
            break;
        }

        case 6: // Watchdog port
            // Arcade machines had a "watchdog" circuit that would reset the
            // machine if the software crashed. The game periodically writes
            // to this port to prevent a reset.
            break;
    }
      
      state->pc += 2;
      break;
    }
  

    // SUI d8 (Subtract Immediate)
    case 0xD6: {

      uint8_t immediate_data = opcode[1];

      uint8_t result = state->a - immediate_data;

      set_zsp_flags(state, result);

      // Set the Carry flag if a borrow occurred.
      state->cc.cy = (state->a < immediate_data);

      // Set the Auxiliary Carry flag if a borrow occurred from the high nibble.
      state->cc.ac = ((state->a & 0x0F) < (immediate_data & 0x0F));
      
      state->a = result;

      state->pc += 2;
      break;
    }

    // CNC a16 (Call on no carry)
    case 0xD4: {
      // call subroutine at a16 if no carry (carry flag == 0)
      if (state->cc.cy == 0) {
        // get 16-bit memory address and save next pc 
        uint16_t address = (opcode[2] << 8) | opcode[1];
        uint16_t pc_return = state-> pc + 3;

        // push return address onto stack
        state->memory[state->sp - 1] = (pc_return >> 8) & 0xff;  // high byte
        state->memory[state->sp - 2] = pc_return & 0xff; // low byte
        state->sp -= 2;

        // jump to target address
        state->pc = address;
      } else {
        state->pc += 3;
      }

      break;
    }

    // RC (Return on Carry)
    case 0xD8: { 

      // Check if the Carry flag (cy) is set (equal to 1).
      if (state->cc.cy == 1) {
        // need to return
        // Pop the 16-bit return address from the stack.
        uint8_t pcl = state->memory[state->sp];       
        uint8_t pch = state->memory[state->sp + 1];   
        
        // Reconstruct the full 16-bit address.
        uint16_t return_address = (pch << 8) | pcl;

        // Set the program counter to the return address.
        state->pc = return_address;
        
        // Adjust the stack pointer because we popped two bytes.
        state->sp += 2;
        
      } else {
        // no return
        state->pc += 1;
      }
      break;
    }

    // JC a16 - Jump if CY flag = 1
    // no flags affected
    case 0xDA: {
      // check if carry flag is set (1 = set)
      if (state->cc.cy == 1) {
        // jump taken: reconstruct 16-bit address from little endian bytes
        uint16_t address = (opcode[2] << 8) | (opcode[1]); // high byte | low byte
        state->pc = address;
      } else {
        // jump not taken: skip over 3-byte instruction (opcode + 2 addres bytes)
        state->pc += 3;
      }
      break;
    }

    // IN instruction
    case 0xDB: { // IN port
      uint8_t port_number = opcode[1]; // The port number is the second byte

      // We check which port the game is asking for
      if (port_number == 1) {
          // If it's Port 1, we load the value from our machine state
          // into the accumulator register.
          state->a = machine->port1;
      }
      else if (port_number == 2) {
          // Port 2 - Player 2 controls and DIP switches
          state->a = 0x00; // Default DIP switches (3 lives, bonus at 1500)
      }
      else if (port_number == 3) {
          // Port 3 is for reading the shift register result.
          uint16_t combined = (machine->shift_register >> (8 - machine->shift_offset)) & 0xFF;
          state->a = combined;
      }

      state->pc += 2;
      break;
    }

    // PUSH D - Push register pair D & E on stack
    // no flags affected
    case 0xD5: {
      uint8_t rph = state->d; // high-order register
      uint8_t rpl = state->e; // low-order register

      // push high byte first, then low byte
      state->memory[state->sp-1] = rph; // D register to SP-1
      state->memory[state->sp-2] = rpl; // E register to SP-2
      
      // derement stack pointer by 2
      state->sp -= 2;
      
      state->pc += 1;
      break;
    }

    // SBI d8 (Subtract immeidate from with borrow)
    case 0xDE: {
      uint8_t immediate_data = opcode[1];
      uint8_t carry_bit = state->cc.cy;
      uint16_t subtrahend = immediate_data + carry_bit; // Combine what we're subtracting
      
      uint8_t result = state->a - subtrahend;
      
      // Set the standard flags
      set_zsp_flags(state, result);

      // Carry flag
      state->cc.cy = (state->a < subtrahend);

      // borrow logic for AC
      state->cc.ac = ((state->a & 0x0F) < ((immediate_data & 0x0F) + carry_bit));

      state->a = result;
      state->pc += 2;
      break;
    }

    // RPO (Return if parity odd)
    case 0xE0: {
      if (state->cc.p == 0) {
        state->pc = (state->memory[state->sp + 1] << 8) | (state->memory[state->sp]);
        state->sp += 2;
      } else{
        state->pc += 1;
      }
      
      break;
    }

    // POP H (Pop register pair H & L off stack)
    case 0xE1: {
      // read stack pointer position from memory and load to h and l
      state->h = state->memory[state->sp+1];
      state->l = state->memory[state->sp];
      state->sp += 2;  // increment stack pointer (popped 2 bytes from stack)
      state->pc += 1;
      break; 
    }

    // JPO a16
    case 0xE2: {
      if (state->cc.p == 0) {
        state->pc = ((opcode[2] << 8) | (opcode[1]));
      } else {
        state->pc += 3;
      }
      break;
    }
    
    // XTHL (Exchange Top of Stack with H-L)
    case 0xE3: { 

      uint8_t temp;

      // Swap the contents of the L register with the byte at SP.
      temp = state->l;
      state->l = state->memory[state->sp];
      state->memory[state->sp] = temp;

      // Swap the contents of the H register with the byte at SP+1.
      temp = state->h;
      state->h = state->memory[state->sp + 1];
      state->memory[state->sp + 1] = temp;

      state->pc += 1;
      break;
    }

    // PUSH H (Push register pair H & L on stack)
    case 0xE5: {
      state->memory[state->sp - 1] = state->h;
      state->memory[state->sp - 2] = state->l;

      state->sp = state->sp - 2;
      state->pc += 1;
      break;
    }

    // ANI - S, Z, A, P, C - (Logical AND accumulator and immediate 8-bit data)
    case 0xE6: {
      uint8_t result  = state->a & opcode[1];   // Logical AND.
      state->cc.s = ((result & 0x80) != 0);     // Logical AND with 1000 0000 to check sign bit.
      state->cc.z = (result == 0);              // Check if result is zero.
      state->cc.p = parity(result);             // Check parity using helper function.
      state->a = result;
      state->cc.cy = 0;                         // Clear carry flag.
      state->cc.ac = 0;                         // Clear auxiliary carry flag. Not used in Space Invaders
      state->pc += 2;
      break;
    }

    // PCHL (Load PC from H-L)
    case 0xE9: { 

      // Combine the H and L registers to form a 16-bit address.
      uint16_t jmp_address = (state->h << 8) | state->l;
      
      state->pc = jmp_address;

      break;
    }

    // XCHG - Exchange H and L with D and E
    // no flags affected
    case 0xEB: {
      // exchange H and D using temporary variable
      uint8_t tempreg = state->h;
      state->h = state->d;
      state->d = tempreg;

      // exchange L and E using temporary variable
      tempreg = state->l;
      state->l = state->e;
      state->e = tempreg;

      state->pc += 1;
      break;
    }

    // CPE a16 (Call parity on even)
    case 0xEC: {
      if (state->cc.p == 1) {
        uint16_t return_address = state->pc + 2;
        state->memory[state->sp - 1] = (return_address >> 8) & 0xff;
        state->memory[state->sp - 2] = return_address & 0xff;
        state->sp -= 2;
        state->pc = (opcode[2] << 8) | opcode[1];
      } else {
        state->pc += 3;
      }
      break;
    }
    
    // XRI d8 (Exclusive OR immediate with A)
    case 0xEE: {
      uint8_t result = state->a = state->a ^ opcode[1];
      state->cc.s = ((result & 0x80) != 0);
      state->cc.z = (result == 0);
      state->cc.p = parity(result);
      state->a = result;
      state->cc.cy = 0;
      state->cc.ac = 0;
      state->pc += 2;
      break;
    }

    // RP (Return on positive)
    case 0xF0: {
      if (state->cc.s == 0) {
        state->pc = (state->memory[state->sp + 1] << 8) | (state->memory[state->sp]);
        state->sp += 2;
      }
      state->pc += 1;
      break;
    }

    // POP PSW (Pop A and Flags off stack)
    // Updates S, Z, A, P, C flags
    case 0xF1: 
    {
      // pop flags
      uint8_t saved_flag_register = state->memory[state->sp];
      
      state->cc.s = (saved_flag_register & 0x80) != 0;  // sign (bit 7)
      state->cc.z = (saved_flag_register & 0x40) != 0;  // zero (bit 6)
      state->cc.ac = (saved_flag_register & 0x10) != 0;  // auxiliary carry (bit 4)
      state->cc.p = (saved_flag_register & 0x04) != 0;  // parity (bit 2)
      state->cc.cy = (saved_flag_register & 0x01) != 0;  // carry (bit 0)

      // pop accumulator
      state->a = state->memory[state->sp+1];

      // update stack pointer
      state->sp += 2;

      state->pc += 1;
      break;
    }
      
    // PUSH PSW (Push A and Flags on stack)
    case 0xF5: {
      state->memory[state->sp - 1] = state->a;

      // Start with the base value. The only bit that is always 1 is bit 1.
      uint8_t flags = 0x02; // Binary 00000010
      flags = flags | (state->cc.cy << 0); // Bit 0
      flags = flags | (state->cc.p  << 2); // Bit 2
      flags = flags | (state->cc.ac << 4); // Bit 4
      flags = flags | (state->cc.z  << 6); // Bit 6
      flags = flags | (state->cc.s  << 7); // Bit 7

      state->memory[state->sp - 2] = flags;
      
      state->sp = state->sp - 2;
      state->pc += 1;
      break;
    }

    // ORI d8 (Inclusive OR immediate with A)
    case 0xF6: {
      uint8_t result = state->a = state->a | opcode[1];
      state->cc.s = ((result & 0x80) != 0);
      state->cc.z = (result == 0);
      state->cc.p = parity(result);
      state->a = result;
      state->cc.cy = 0;
      state->cc.ac = 0;
      state->pc += 2;
      break;
    }

    // RM (Return on minus)
    case 0xF8: {
      if (state->cc.s == 1) {
        state->pc = (state->memory[state->sp + 1] << 8) | (state->memory[state->sp]);
        state->sp += 2;
      } else{
        state->pc += 1;
      }
      break;
    }

    // JM addr (Jump on Minus/Sign)
    case 0xFA: { 

      // Check if the Sign flag is set.
      if (state->cc.s == 1) {
        // Get the 16-bit call address from the next two bytes.
        uint16_t jmp_address = (opcode[2] << 8) | opcode[1];
        
        // Set the program counter to the new address.
        state->pc = jmp_address;

      } else {
        // Simply advance the program counter
        state->pc += 3;
      }
      break;
    }

    // EI (Enable interruprt)
    case 0xFB: {
        state->int_enable = 1;
        state->pc += 1;
        break;
    }

    // CM addr (Call on Minus/Sign)
    case 0xFC: { 

      // Check if the Sign flag is set
      if (state->cc.s == 1) {
        // Get the 16-bit call address from the next two bytes.
        uint16_t call_address = (opcode[2] << 8) | opcode[1];

        // The return address is the one after this 3-byte instruction.
        uint16_t return_address = state->pc + 3;

        // Push the return address onto the stack.
        state->memory[state->sp - 1] = (return_address >> 8) & 0xFF;
        state->memory[state->sp - 2] = return_address & 0xFF;        
        state->sp -= 2;

        // Jump to the subroutine address.
        state->pc = call_address;
        
      } else {
        // Simply advance the program counter
        state->pc += 3;
      }
      break;
    }
    
    // CPI d8 (Compare Immediate 8-bit Data with Accumulator)
    // Updates S, Z, A, P, C flags
    case 0xFE:
    {
      // compute (a - d8) for compare
      uint8_t diff = state->a - opcode[1];
      
      // set flags based on diff
      set_zsp_flags(state, diff);  // zero, sign, parity
      state->cc.cy = (state->a < opcode[1]); // carry (borrow from MSB)
      state->cc.ac = ((state->a & 0x0F) < (opcode[1] & 0x0F));  // auxiliary carry (borrow between nibbles)

      state->pc += 2;

      break;
    }

    
    // RST 7 (Restart 7)
    case 0xFF: { 

      // the return address, which is the instruction after this one.
      uint16_t return_address = state->pc + 1;

      // Push the return address onto the stack
      state->memory[state->sp - 1] = (return_address >> 8) & 0xFF;
      state->memory[state->sp - 2] = return_address & 0xFF;     
      
      // Decrement the stack pointer.
      state->sp -= 2;

      // Jump to the fixed RST 7 address (7 * 8 = 0x38).
      state->pc = 0x38;

      break;
    }
      
    // default (unimplemented instructions)
    default:
      printf("Unimplemented instruction 0x%02x at PC=0x%04x\n", *opcode, state->pc);
      exit(1);
  }
}

// Interrupt helper, PUSH PC, similar to other push instructions.
// Adapted from https://web.archive.org/web/20240118230840/http://www.emulator101.com/interrupts.html
void push_pc(State8080* state, uint16_t pc) {
  state->memory[state->sp - 1] = (pc >> 8) & 0xff;    // Set higher order byte on stack
  state->memory[state->sp - 2] = pc & 0xff;           // Set lower order byte on stack. 

  state->sp -= 2;
}

void generateInterrupt(State8080* state, int interrupt_num) {
  
  // An interrupt is done if the interrupt flag is enabled.
  if (state->int_enable == 0) {
      return;
  }
  
  // perform "PUSH PC"
  push_pc(state, state->pc);

  // Set the PC to the low memory vector.
  // This is identail to an "RST" interrupt" instruction.
  state->pc = 8 * interrupt_num;

  // disable interrupt after it is done
  state->int_enable = 0;
  }
 
int main(int argc, char** argv) {
  // TODO: add command-line argc/argv argument handling
  if (argc != 2) {
    err(1, "Error, invalid command line arguments");
  }

  // Open ROM from command line
  FILE *fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    err(1, "Unable to read ROM file: %s\n", argv[1]);
  } else {
    printf("ROM file opened successfully: %s\n", argv[1]);
  }

  // initialize 8080 CPU state
  State8080* state = (State8080*)calloc(1, sizeof(State8080));
  if(state == NULL) {
    fprintf(stderr, "Failed to allocate CPU State\n");
    return 1;
  }
  
  // Initialize memory
  state->memory = (uint8_t*) calloc(MEMORY_SIZE, sizeof(uint8_t));
  if(state->memory == NULL) {
    fprintf(stderr, "Failed to allocate memory\n");
    free(state);
    return 1;
  }

  // Initialize the machine state
  MachineState* machine = (MachineState*)calloc(1, sizeof(MachineState));
  if(machine == NULL) {
    fprintf(stderr, "Failed to allocate Machine State\n");
    free(state->memory);
    free(state);
    return 1;
  }

  // Initialize port1. Bit 3 must always be 1.
  machine->port1 = 0b00001000;
  machine->port2 = 0x00;  // Can set DIP switches here

  // Find file size. Space Invaders is 8192 bytes.
  fseek(fp, 0L, SEEK_END);
  long int file_size = ftell(fp);
  // Set pointer back to beginning of file.
  fseek(fp, 0L, SEEK_SET);

  // Read ROM into state->memory;
  size_t bytes_read = fread(state->memory, sizeof(uint8_t), file_size, fp);
  printf("bytes read: %ld\n", (size_t) bytes_read);

  // Initialize graphics (this now handles SDL_Init and the window)
  if (!graphics_init()) {
      fprintf(stderr, "Graphics initialization failed.\n");
      return 1;
  }

  // Initialize sound
  if (!sound_init()) {
    fprintf(stderr, "Sound initialization failed.\n");
    return 1;
}


  // Close file
  fclose(fp);

  // --- Main Emulation Loop ---
  uint32_t nxt_interrupt_time = SDL_GetTicks();

  int which_interrupt = 1; // Start with the mid-screen interrupt (RST 1)
  
  bool quit = false;
  
  while (!quit) {
      // 1. Handle user input and events (check for quit)
      if (io_handle_input(machine) != 0) {
          quit = true;
      }  
      
      //print_state_code(state);
      uint32_t current_time = SDL_GetTicks(); // milliseconds since SDL_Init()
      
      if (current_time > nxt_interrupt_time) { 
        // start interrupt 1 or 2
        generateInterrupt(state, which_interrupt); 
        
        // V blank interrupt (RST 2) is when to draw the screen
        if (which_interrupt == 2) {
              graphics_draw(state->memory);
        }
        
        // now flip the interrupt
        if (which_interrupt == 1){
          which_interrupt = 2;
        }
        else{
          which_interrupt = 1;
        }
        
        // set the timer for next interrupt, 8ms (half of 16ms)
        nxt_interrupt_time = SDL_GetTicks() + 8;
        
      }
      
      // 2. Emulate small chunk of the CPU instructions
      for (int i = 0; i < 100; i++) {
        Emulate8080Op(state, machine);
      }
      
  }

  // --- Cleanup Phase ---
  graphics_cleanup(); // This now handles SDL_Quit and destroys the window
  sound_cleanup();

  free(state->memory);
  free(state);
  free(machine);

  return 0;
}
