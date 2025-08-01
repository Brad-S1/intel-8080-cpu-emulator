#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <err.h>
#include "disassembler.c"

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
  printf("SP: %04x, PC: %04x - ", sp, pc);
  
  uint8_t a = state->a;
  uint8_t b = state->b;
  uint8_t c = state->c;
  uint8_t d = state->d;
  uint8_t e = state->e;
  uint8_t h = state->h;
  uint8_t l = state->l;

  // Prints all 8-bit general registers.
  printf("A: %02x, B: %02x, C: %02x, D: %02x, E: %02x, H: %02x, L: %02x - ", a, b, c, d, e, h, l);

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
void Emulate8080Op(State8080* state) {
  // pointer to memory at program counter address position
  uint8_t* opcode = &state->memory[state->pc];
  disassembled8080Op(state->memory, state->pc);

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

    // INX D (Increment D & E 16-bit register pair)
    case 0x13: {
      uint16_t de_pair = (state->d << 8) | state->e;
      
      de_pair++;

      state->d = (de_pair >> 8) & 0xFF; // The high byte goes back to D
      state->e = de_pair & 0xFF;        // The low byte goes back to E

      state->pc += 1;
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

    
    // LXI H,d16 (Load Register Pair H and L Immediate 16-bit Data)
    case 0x21: {
      state->h = opcode[2];
      state->l = opcode[1];
      state->pc +=3;
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

    // MVI H (Load immediate 8-bit data to register H)
    case 0x26: {
      state->h = opcode[1];   // Load register H with contents of byte 2; 
      state->pc += 2;
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

    // MVI M (Load immediate 8-bit data to registers H and L)           
    case 0x36: {
      uint16_t address = (state->h << 8) | (state->l);  // Create memory address from registers h and l.
      state->memory[address] = opcode[1];               // Move immediate 8-bit data to that address.
      state->pc += 2;
      break;
    }
      
    // LDA (Load accumulator direct 16-bit data)
    case 0x3A: {
      uint16_t address = (opcode[2] << 8) | (opcode[1]);    // Create memory address from bytes 3 and 2.
      state->a = state->memory[address];                    // Load register a with contents of memory address.
      state->pc += 3;
      break;
    }
      
    // MVI A (Move immediate 8-bit data to Accumulator)
    case 0x3E: {
      state->a = opcode[1];   // Load register A with contents of byte 2;
      state->pc += 2;
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
    
    // MOV E,M (Move Data from Memory (addressed by H and L) to Register E)
    case 0x5E: {
      // access 16-bit memory address at HL register pair
      state->e = state->memory[(state->h <<8 | state->l)];
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

    // MOV L,A (Move data from accumulator to register l)
    case 0x6F: {
      state->l = state->a;
      state->pc += 1;
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
    
    // MOV A,M (Move Data from Memory (addressed by H and L) to Accumulator)
    case 0x7E: {
      state->a = state->memory[(state->h << 8 | state->l)];
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

    // POP D (Pop register pair D & E off stack)
    case 0xD1: {
      state->d = state->memory[state->sp + 1];
      state->e = state->memory[state->sp];
      
      state->sp = state->sp + 2;
      state->pc += 1;
      break;
    }

    // OUT (Output immediate 8-bit to port)     SKIP FOR NOW.
    case 0xD3: {
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

    // POP H (Pop register pair H & L off stack)
    case 0xE1: {
      // read stack pointer position from memory and load to h and l
      state->h = state->memory[state->sp+1];
      state->l = state->memory[state->sp];
      state->sp += 2;  // increment stack pointer (popped 2 bytes from stack)
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

    // EI (Enable interruprt)
    case 0xFB: {
        state->int_enable = 1;
        state->pc += 1;
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
      
    // default (unimplemented instructions)
    default:
      printf("Unimplemented instruction 0x%02x at PC=0x%04x\n", *opcode, state->pc);
      exit(1);
  }
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

  // Find file size. Space Invaders is 8192 bytes.
  fseek(fp, 0L, SEEK_END);
  long int file_size = ftell(fp);
  // Set pointer back to beginning of file.
  fseek(fp, 0L, SEEK_SET);

  // Read ROM into state->memory;
  size_t bytes_read = fread(state->memory, file_size, sizeof(uint8_t), fp);
  printf("bytes read: %ld\n", (size_t) bytes_read);

  // Close file
  fclose(fp);

  while(1) {
    Emulate8080Op(state);  
    print_state_code(state);
  }

  return 0;
}
